#include "codes/codes.hpp"
#include "codes/tools/magic_enum.hpp"

#include "modules/base_module.hpp"
#include <cstring>
#include <string>

char str_buffer[32];

/**
 * @brief Writes the given string to a global buffer
 *        use to extend the life of std::string.
 *        Truncates the string if it exceeds 31 characters.
 * @param str the string that should be written into the buffer
 */
void write_to_buffer(std::string str){
    size_t length = str.length();
    
    if (length > 31) {
        length = 31;
    }
    
    std::memcpy(str_buffer, str.data(), length);
    
    str_buffer[length] = '\0';
}

std::string Generate_product_name_cpp(){
    const Codes::Module module_type = Base_module::Module_type();
    
    std::string module_type_str = std::string{magic_enum::enum_name(module_type)};
    return "SMPBR - " + module_type_str;
}

std::string Generate_device_serial_cpp(){
    const UID_t uid = Base_module::UID();
    std::string output = "";
    
    for(const auto& uid_part : uid){
        output += emio::format("{:x}",uid_part);
    }
    
    return output;
}

extern "C" const char* Generate_product_name() {
    write_to_buffer(Generate_product_name_cpp());
    return str_buffer;
}

extern "C" const char* Generate_device_serial(){
    write_to_buffer(Generate_device_serial_cpp());
    return str_buffer;
}