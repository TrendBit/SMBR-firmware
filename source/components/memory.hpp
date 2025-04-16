/**
 * @file memory.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 07.04.2025
 */


#pragma once

#include "components/memory/M24Cxx.hpp"
#include "codes/codes.hpp"
#include "logger.hpp"
#include "rtos/wrappers.hpp"

#include <cstdint>
#include <unordered_map>
#include <optional>
#include <vector>
#include <string>

/**
 * @brief EEPROM memory provider for storing and reading persistent data data
 */
class EEPROM_storage {
public:
    /**
     * @brief Name of records stored in EEPROM
     */
    enum class Record_name {
        Module_type,
        Instance_enumeration,
        Reserved,
        OJIP_calibration,
        SPM_nominal_calibration, // Spectrophotometer
    };

private:

    /**
     * @brief Pointer to EEPROM memory
     */
    M24Cxx * const eeprom;

    /**
     * @brief  Structure which is holding information about record in EEPROM
     */
    struct Record {
        uint32_t offset = 0;
        uint16_t length = 0;
    };

    /**
     * @brief   Mapping between record name and its location in EEPROM
     *          In future should even contain in which EEPROM chip
     *          Array of pairs because constexpr std::map does not exist in c++20
     */
    static constexpr std::array<std::pair<Record_name, Record>, 5> records = {
        std::make_pair(Record_name::Module_type,                    Record{0x0000, 1}),
        std::make_pair(Record_name::Instance_enumeration,           Record{0x0001, 1}),
        std::make_pair(Record_name::Reserved,                       Record{0x0002, 2}),
        std::make_pair(Record_name::SPM_nominal_calibration,        Record{0x0300, 24}),
        std::make_pair(Record_name::OJIP_calibration,               Record{0x0400, 2000}),
    };

public:
    /**
     * @brief Construct a new eeprom storage object
     *
     * @param eeprom     Pointer to EEPROM memory on Mainboard
     * @param module     Module type of this instance
     * @param instance   Instance enumeration of this instance
     */
    explicit EEPROM_storage(M24Cxx * const eeprom);


    bool Check_type(Codes::Module module, Codes::Instance instance);

    /**
     * @brief   Read record of module type from EEPROM
     *
     * @return Codes::Module        Type of module which is saved in EEPROM
     */
    Codes::Module Module();

    /**
     * @brief   Read record of instance enumeration from EEPROM
     *
     * @return Codes::Instance      Instance enumeration of module which is saved in EEPROM
     */
    Codes::Instance Instance();

    /**
     * @brief   Read OJIP calibration data from EEPROM
     *
     * @param calibration   Location where calibration data will be stored
     * @return true         Data was read successfully
     * @return false        Data was not read, memory not accessible
     */
    bool Read_OJIP_calibration(std::array<uint16_t, 1000> &calibration);

    /**
     * @brief   Write OJIP calibration data to EEPROM
     *          This is 1000 of ADC samples from fluorometer taken at 50x gain
     *
     * @param calibration   Calibration data to be written to EEPROM
     * @return true         Data was written successfully
     * @return false        Data was not written, memory not accessible
     */
    bool Write_OJIP_calibration(std::array<uint16_t, 1000> &calibration);

    /**
     * @brief   Read spectrophotometer calibration data from EEPROM
     *
     * @param calibration   Location where calibration data will be stored
     * @return true         Data was read successfully
     * @return false        Data was not read, memory not accessible or data not valid (empty)
     */
    bool Read_spectrophotometer_calibration(std::array<float, 6> &calibration);

    /**
     * @brief   Write spectrophotometer calibration data to EEPROM
     *
     * @param calibration   Calibration data to be written to EEPROM
     * @return true         Data was written successfully
     * @return false        Data was not written, memory not accessible
     */
    bool Write_spectrophotometer_calibration(std::array<float, 6> &calibration);

private:
    /**
     * @brief   Format all records in EEPROM to 0xff
     *
     * @return true     All records were formatted successfully
     * @return false    At least one record was not formatted
     */
    bool Format_records();

    /**
     * @brief   Compile time check to ensure that records are not overlapping
     *
     * @return true     Records are not overlapping
     * @return false    Records are overlapping
     */
    static constexpr bool Check_for_overlapping_records();

    /**
     * @brief   Compile time check to ensure that there are no gaps between records
     *
     * @return true     There are no gaps between records
     * @return false    There are gaps between records
     */
    static constexpr bool Check_for_record_gaps();

    /**
     * @brief   Read record from EEPROM from position determined by record name
     *          Return only raw data, no interpretation
     *
     * @param name  Name of record to be read
     * @return std::optional<std::vector<uint8_t>>  Data read from EEPROM if available
     */
    std::optional<std::vector<uint8_t>> Read_record(Record_name name);

    /**
     * @brief   Write data to EEPROM to position determined by record name
     *
     * @param name  Name of record to be written
     * @param data  Data to be written to EEPROM
     * @return true     Data was written successfully
     * @return false    Data was not written, memory not accessible
     */
    bool Write_record(Record_name name, std::vector<uint8_t> &data);
};

