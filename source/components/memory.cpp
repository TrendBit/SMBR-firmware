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

bool EEPROM_storage::Check_type(Codes::Module module, Codes::Instance instance){
    bool type_check = true;

    // Check if module is in program is same as in EEPROM
    Codes::Module current_module = Module();
    if(current_module != Codes::Module::Undefined) {
        Logger::Print("EEPROM storage already contains module", Logger::Level::Trace);
        if(current_module != module) {
            Logger::Print("EEPROM storage contains data for another module", Logger::Level::Error);
            type_check = false;
        } else {
            Logger::Print("EEPROM storage contains data for the same module", Logger::Level::Trace);
        }
    } else {
        Logger::Print("EEPROM does not contain module type", Logger::Level::Error);
        Logger::Print("Reseting module type in EEPROM", Logger::Level::Trace);
        std::vector<uint8_t> data = {static_cast<uint8_t>(module)};
        Write_record(Record_name::Module_type, data);
        type_check = false;
        rtos::Delay(5);
    }

    // Check if instance is in program is same as in EEPROM
    Codes::Instance current_instance = Instance();
    if(current_instance != Codes::Instance::Undefined) {
        Logger::Print("EEPROM storage already contains instance", Logger::Level::Trace);
        if(current_instance != instance) {
            Logger::Print("EEPROM storage contains data for another instance", Logger::Level::Error);
            type_check = false;
        } else {
            Logger::Print("EEPROM storage contains data for the same instance", Logger::Level::Trace);
        }
    } else {
        Logger::Print("EEPROM does not contain instance enumeration", Logger::Level::Error);
        Logger::Print("Reseting instance enumeration in EEPROM", Logger::Level::Trace);
        std::vector<uint8_t> data = {static_cast<uint8_t>(instance)};
        Write_record(Record_name::Instance_enumeration, data);
        type_check = false;
        rtos::Delay(5);
    }

    return type_check;
}

bool EEPROM_storage::Write_OJIP_calibration(std::array<uint16_t, 1000> &calibration) {
    Record_name name = Record_name::OJIP_calibration;
    auto it = std::find_if(records.begin(), records.end(),
        [name](const auto& pair) { return pair.first == name; });
    if (it == records.end()) {
        Logger::Print("Record not found", Logger::Level::Error);
        return false; // Record with this name was not found
    }

    const Record& record = it->second;
    uint16_t start_address = record.offset;
    uint16_t total_length = record.length * 2;  // Each uint16_t is 2 bytes

    if (total_length < sizeof(uint16_t) * calibration.size()) {
        Logger::Print("Calibration too large for EEPROM record", Logger::Level::Error);
        return false;
    }

    const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(calibration.data());

    constexpr size_t CHUNK_SIZE = 32;  // 32 bytes per write
    size_t bytes_written = 0;
    bool status = true;

    // Split data into chunks with siz similar to EEPROM page size
    while (bytes_written < total_length && status) {
        // Calculate current chunk size (may be smaller for the last chunk)
        size_t current_chunk_size = std::min(CHUNK_SIZE, total_length - bytes_written);
        uint16_t current_address = start_address + bytes_written;

        status = eeprom->Write(current_address, data_ptr + bytes_written, current_chunk_size);

        if (!status) {
            Logger::Print(emio::format("EEPROM write failed at address 0x{:04x}", current_address), Logger::Level::Error);
            return false;
        }

        bytes_written += current_chunk_size;

        // Delay to allow EEPROM write cycle to complete
        rtos::Delay(5);
    }

    Logger::Print(emio::format("EEPROM write succeeded, {} bytes written in {} chunks",
               total_length, (total_length + CHUNK_SIZE - 1) / CHUNK_SIZE), Logger::Level::Notice);
    return true;
}

bool EEPROM_storage::Read_OJIP_calibration(std::array<uint16_t, 1000> &calibration) {
    Record_name name = Record_name::OJIP_calibration;
    auto it = std::find_if(records.begin(), records.end(),
        [name](const auto& pair) { return pair.first == name; });
    if (it == records.end()) {
        Logger::Print("Record not found", Logger::Level::Error);
        return false;
    }

    const Record& record = it->second;
    uint16_t start_address = record.offset;
    uint16_t total_length = record.length;

    if (total_length < sizeof(uint16_t) * calibration.size()) {
        Logger::Print("Calibration record size too small", Logger::Level::Error);
        return false;
    }

    uint8_t* data_ptr = reinterpret_cast<uint8_t*>(calibration.data());

    constexpr size_t CHUNK_SIZE = 32;  // 32 bytes per read
    size_t bytes_read = 0;
    bool status = true;

    while (bytes_read < total_length && status) {
        // Calculate current chunk size
        size_t current_chunk_size = std::min(CHUNK_SIZE, total_length - bytes_read);
        uint16_t current_address = start_address + bytes_read;

        auto chunk_data = eeprom->Read(current_address, current_chunk_size);

        if (!chunk_data.has_value()) {
            Logger::Print(emio::format("EEPROM read failed at address 0x{:04x}", current_address), Logger::Level::Error);
            return false;
        }

        std::copy(chunk_data->begin(), chunk_data->end(), data_ptr + bytes_read);
        bytes_read += current_chunk_size;
    }

    Logger::Print(emio::format("EEPROM read succeeded, {} bytes read in {} chunks",
               bytes_read, (bytes_read + CHUNK_SIZE - 1) / CHUNK_SIZE), Logger::Level::Notice);
    return true;
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
