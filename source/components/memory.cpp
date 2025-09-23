#include "memory.hpp"

constexpr bool EEPROM_storage::Check_for_overlapping_records() {
    uint16_t last_end = 0;
    for (const auto& [name, record] : records) {
        if (record.offset < last_end) {
            return false;
        }
        last_end = record.offset + record.length;
    }
    return true;
}

constexpr bool EEPROM_storage::Check_for_record_gaps() {
    uint16_t last_end = 0;
    for (const auto& [name, record] : records) {
        if (record.offset > last_end) {
            return false;
        }
        last_end = record.offset + record.length;
    }
    return true;
}

EEPROM_storage::EEPROM_storage(M24Cxx * const eeprom)
    :eeprom(eeprom)
{
    // Compile time checks
    static_assert(EEPROM_storage::Check_for_overlapping_records(), "Records are overlapping");
}

bool EEPROM_storage::Check_type(Codes::Module module_type, Codes::Instance module_instance){
    bool type_check = true;

    // Check if module is in program is same as in EEPROM
    Codes::Module memory_module = Module();
    if(memory_module != Codes::Module::Undefined) {
        Logger::Trace("EEPROM storage already contains module");
        if(memory_module != module_type) {
            Logger::Error("EEPROM storage contains signature of another module type: memory {}, module {}",
                          Codes::to_string(memory_module), Codes::to_string(module_type));

            type_check = false;
        } else {
            Logger::Trace("EEPROM storage contains signature of the same module");
        }
    } else {
        Logger::Error("EEPROM does not contain module type");
        Logger::Trace("Reseting module type in EEPROM");
        std::vector<uint8_t> data = {static_cast<uint8_t>(module_type)};
        Write_record(Record_name::Module_type, data);
        type_check = false;
        rtos::Delay(5);
    }

    // Check if instance is in program is same as in EEPROM
    Codes::Instance memory_instance = Instance();
    if(memory_instance != Codes::Instance::Undefined) {
        Logger::Trace("EEPROM storage already contains instance");
        if(memory_instance != module_instance) {
            Logger::Error("EEPROM storage contains signature of another instance type: memory {}, module {}",
                          Codes::to_string(memory_instance), Codes::to_string(module_instance));
            type_check = false;
        } else {
            Logger::Trace("EEPROM storage contains signature of the same instance");
        }
    } else {
        // Real module instance can be undefined at start for non-initialized enumeration modules
        if (module_instance == Codes::Instance::Undefined) {
            Logger::Trace("EEPROM storage contains data for the same instance");
        } else { // Real instance is different from Undefined
            Logger::Error("EEPROM does not contain instance enumeration");
            Logger::Trace("Reseting instance enumeration in EEPROM");
            std::vector<uint8_t> data = {static_cast<uint8_t>(module_instance)};
            Write_record(Record_name::Instance_enumeration, data);
            type_check = false;
            rtos::Delay(5);
        }
    }

    return type_check;
}

bool EEPROM_storage::Write_chunked_data(Record_name name, const uint8_t* data_ptr, size_t data_size_bytes) {
    auto it = std::find_if(records.begin(), records.end(),
        [name](const auto& pair) { return pair.first == name; });
    if (it == records.end()) {
        Logger::Error("Record {} not found for writing", static_cast<int>(name));
        return false;
    }

    const Record& record = it->second;
    uint16_t start_address = record.offset;

    if (record.length < data_size_bytes) {
        Logger::Error("Data ({} bytes) too large for EEPROM record ({} bytes)", data_size_bytes, record.length);
        return false;
    }

    constexpr size_t CHUNK_SIZE = 32;
    size_t bytes_written = 0;
    bool status = true;

    while (bytes_written < data_size_bytes && status) {
        size_t current_chunk_size = std::min(CHUNK_SIZE, data_size_bytes - bytes_written);
        uint16_t current_address = start_address + bytes_written;
        status = eeprom->Write(current_address, data_ptr + bytes_written, current_chunk_size);
        if (!status) {
            Logger::Error("EEPROM write failed at address 0x{:04x}", current_address);
            return false;
        }
        bytes_written += current_chunk_size;
        rtos::Delay(5); // Delay for EEPROM write cycle
    }

    Logger::Debug("EEPROM chunked write succeeded, {} bytes written", bytes_written);
    return true;
}

bool EEPROM_storage::Read_chunked_data(Record_name name, uint8_t* data_ptr, size_t data_size_bytes) {
    auto it = std::find_if(records.begin(), records.end(),
        [name](const auto& pair) { return pair.first == name; });
    if (it == records.end()) {
        Logger::Error("Record {} not found for reading", static_cast<int>(name));
        return false;
    }

    const Record& record = it->second;
    uint16_t start_address = record.offset;

    if (record.length < data_size_bytes) {
        Logger::Error("Record size ({} bytes) too small for requested read ({} bytes)", record.length, data_size_bytes);
        return false;
    }

    constexpr size_t CHUNK_SIZE = 32;
    size_t bytes_read = 0;

    while (bytes_read < data_size_bytes) {
        size_t current_chunk_size = std::min(CHUNK_SIZE, data_size_bytes - bytes_read);
        uint16_t current_address = start_address + bytes_read;
        auto chunk_data = eeprom->Read(current_address, current_chunk_size);
        if (!chunk_data.has_value()) {
            Logger::Error("EEPROM read failed at address 0x{:04x}", current_address);
            return false;
        }
        std::copy(chunk_data->begin(), chunk_data->end(), data_ptr + bytes_read);
        bytes_read += current_chunk_size;
    }

    Logger::Debug("EEPROM chunked read succeeded, {} bytes read", bytes_read);
    return true; // Return status based on loop completion
}

bool EEPROM_storage::Write_OJIP_calibration_values(std::array<uint16_t, FLUOROMETER_CALIBRATION_SAMPLES> &calibration) {
    const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(calibration.data());
    size_t data_size_bytes = sizeof(uint16_t) * calibration.size();
    // Call helper without the string argument
    if (Write_chunked_data(Record_name::OJIP_calibration_values, data_ptr, data_size_bytes)) {
        Logger::Debug("EEPROM ADC calibration write succeeded.");
        return true;
    } else {
        Logger::Error("EEPROM ADC calibration write failed.");
        return false;
    }
}

bool EEPROM_storage::Write_OJIP_calibration_timing(std::array<uint32_t, FLUOROMETER_CALIBRATION_SAMPLES> &calibration_timing) {
    const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(calibration_timing.data());
    size_t data_size_bytes = sizeof(uint32_t) * calibration_timing.size();
    // Call helper without the string argument
    if (Write_chunked_data(Record_name::OJIP_calibration_timing, data_ptr, data_size_bytes)) {
        Logger::Debug("EEPROM timing calibration write succeeded.");
        return true;
    } else {
        Logger::Error("EEPROM timing calibration write failed.");
        return false;
    }
}

bool EEPROM_storage::Is_data_valid(const uint8_t* data_ptr, size_t data_size_bytes) {
    if (data_size_bytes == 0) {
        return false; // No data are invalid
    }

    bool all_zero = true;
    bool all_ff = true;

    for (size_t i = 0; i < data_size_bytes; ++i) {
        if (data_ptr[i] != 0x00) {
            all_zero = false;
        }
        if (data_ptr[i] != 0xFF) {
            all_ff = false;
        }
        if (!all_zero && !all_ff) {
            return true; // Data are mixed -> valid
        }
    }

    return !(all_zero || all_ff);
}

bool EEPROM_storage::Read_OJIP_calibration_values(std::array<uint16_t, FLUOROMETER_CALIBRATION_SAMPLES> &calibration) {
    uint8_t* data_ptr = reinterpret_cast<uint8_t*>(calibration.data());
    size_t data_size_bytes = sizeof(uint16_t) * calibration.size();

    if (!Read_chunked_data(Record_name::OJIP_calibration_values, data_ptr, data_size_bytes)) {
        Logger::Error("EEPROM ADC calibration read failed during chunk transfer.");
        return false; // Read failed
    }

    if (!Is_data_valid(data_ptr, data_size_bytes)) {
        Logger::Warning("EEPROM ADC calibration data not initialized.");
        return false; // Data invalid
    }

    Logger::Debug("EEPROM ADC calibration read succeeded and appears valid.");
    return true; // Read succeeded and data is valid
}

bool EEPROM_storage::Read_OJIP_calibration_timing(std::array<uint32_t, FLUOROMETER_CALIBRATION_SAMPLES> &calibration_timing) {
    uint8_t* data_ptr = reinterpret_cast<uint8_t*>(calibration_timing.data());
    size_t data_size_bytes = sizeof(uint32_t) * calibration_timing.size();

    if (!Read_chunked_data(Record_name::OJIP_calibration_timing, data_ptr, data_size_bytes)) {
         Logger::Error("EEPROM timing calibration read failed during chunk transfer.");
        return false; // Read failed
    }

    if (!Is_data_valid(data_ptr, data_size_bytes)) {
        Logger::Warning("EEPROM timing calibration data not initialized.");
        return false; // Data invalid
    }

    Logger::Debug("EEPROM timing calibration read succeeded and appears valid.");
    return true; // Read succeeded and data is valid
}

bool EEPROM_storage::Read_spectrophotometer_calibration(std::array<float, 6> &calibration){
    auto record = Read_record(Record_name::SPM_nominal_calibration);
    if (record.has_value()) {
        if (record.value()[0] == 0xFF and record.value()[1] == 0xFF) {
            return false; // Invalid values of data
        } else {
            std::copy(record.value().begin(), record.value().end(), reinterpret_cast<uint8_t*>(calibration.data()));
            return true;
        }
    } else {
        return false; // Data not available, read failed
    }
}

bool EEPROM_storage::Write_spectrophotometer_calibration(std::array<float, 6> &calibration){
    std::vector<uint8_t> data;
    data.resize(sizeof(float) * calibration.size());
    std::copy(reinterpret_cast<uint8_t*>(calibration.data()), reinterpret_cast<uint8_t*>(calibration.data()) + sizeof(float) * calibration.size(), data.begin());
    return Write_record(Record_name::SPM_nominal_calibration, data);
}

Codes::Module EEPROM_storage::Module(){
    auto record = Read_record(Record_name::Module_type);
    if (record.has_value()) {
        if (record->data()[0] == 0xFF) {
            return Codes::Module::Undefined;
        } else {
            return static_cast<Codes::Module>(record->data()[0] & 0xff);
        }
    } else {
        return Codes::Module::Undefined;
    }
}

Codes::Instance EEPROM_storage::Instance(){
    auto record = Read_record(Record_name::Instance_enumeration);
    if (record.has_value()) {
        if (record->data()[0] == 0xFF) {
            return Codes::Instance::Undefined;
        } else {
            return static_cast<Codes::Instance>(record->data()[0] & 0x0f);
        }
    } else {
        return Codes::Instance::Undefined;
    }
}

bool EEPROM_storage::Instance(const Codes::Instance &instance){
    std::vector<uint8_t> data = {static_cast<uint8_t>(instance)};
    return Write_record(Record_name::Instance_enumeration, data);
}

std::optional<std::vector<uint8_t>> EEPROM_storage::Read_record(EEPROM_storage::Record_name name){
    auto it = std::find_if(records.begin(), records.end(),
        [name](const auto& pair) { return pair.first == name; });
    if (it == records.end()) {
        return {}; // Record with this name was not found
    }

    const Record& record = it->second;

    auto data = eeprom->Read(record.offset, record.length);

    if (data.has_value()) {
        return data;
    } else {
        return {};
    }
}

bool EEPROM_storage::Write_record(EEPROM_storage::Record_name name, std::vector<uint8_t> &data){
    auto it = std::find_if(records.begin(), records.end(),
        [name](const auto& pair) { return pair.first == name; });
    if (it == records.end()) {
        return false; // Record with this name not found
    }
    const Record& record = it->second;

    if (data.size() != record.length) {
        return false; // Data size does not match record length
    }
    return eeprom->Write(record.offset, data);
}


bool EEPROM_storage::Format_records() {
    for (const auto& [name, record] : records) {
        std::vector<uint8_t> data(record.length, 0xff); // Formated vector
        if (!eeprom->Write(record.offset, data)) {
            return false; // Write failed
        }
    }
    return true; // All writes succeeded
}
