#include "base_module.hpp"

#include "cli.hpp"
#include "codes/codes.hpp"
#include "threads/common_thread.hpp"
#include "threads/module_check_thread.hpp" 
#include "module_check/invalid_instance_check.hpp"
#include <cstdint>
#include <vector>

Base_module::Base_module(Codes::Module module_type, Enumerator * const enumerator, uint green_led_pin, uint i2c_sda, uint i2c_scl):
Base_module(module_type, enumerator, green_led_pin, i2c_sda, i2c_scl, std::nullopt)
{

}

Base_module::Base_module(Codes::Module module_type, Enumerator * const enumerator, uint green_led_pin, uint i2c_sda, uint i2c_scl, uint yellow_led_pin):
    Base_module(module_type, enumerator, green_led_pin, i2c_sda, i2c_scl, std::optional<GPIO * const>(new GPIO(yellow_led_pin, GPIO::Direction::Out)))
{

}

Base_module::Base_module(Codes::Module module_type, Enumerator * const enumerator, uint green_led_pin, uint i2c_sda, uint i2c_scl, std::optional<GPIO * const> yellow_led):
    module_type(module_type),
    i2c(new I2C_bus(i2c1, i2c_sda, i2c_scl, 100000, true)),
    memory(new EEPROM_storage(new AT24Cxxx(*i2c, 0x50, 64))),
    adc_mutex(new fra::MutexStandard()),
    can_thread(new CAN_thread()),
    common_thread(new Common_thread(can_thread, memory)),
    common_core(new Common_core(adc_mutex)),
    heartbeat_thread(new Heartbeat_thread(green_led_pin,200)),
    yellow_led(yellow_led),
    version_voltage_channel(new ADC_channel(ADC_channel::RP2040_ADC_channel::CH_0, 3.30f)),
    enumerator(enumerator)
{
    this->singleton_instance = this;

    #ifdef CONFIG_TEST_THREAD
        new Test_thread();
    #endif

    if (yellow_led.has_value()) {
        yellow_led.value()->Set(true);
    }
    module_check_thread = new Module_check_thread();

    if(enumerator){
        module_check_thread->AttachCheck(new Invalid_instance_check(this,enumerator));
    }

    base_module_cli = new Base_module_cli(this);
}

Codes::Module Base_module::Module_type() {
    if (Singleton_instance()) {
        return Singleton_instance()->module_type;
    } else {
        return Codes::Module::Undefined;
    }
}

Codes::Instance Base_module::Instance_enumeration() {
    if (Singleton_instance()) {
        return Singleton_instance()->enumerator->Instance();
    } else {
        return Codes::Instance::Undefined;
    }
}

Base_module * Base_module::Singleton_instance(){
    return singleton_instance;
}

UID_t Base_module::UID(){
    std::array<uint8_t, PICO_UUID_LEN> pico_uid;
    UID_t fast_hash_uid;

    pico_get_unique_board_id((pico_unique_board_id_t*)pico_uid.data());
    uint64_t hash = fasthash64(pico_uid.data(), PICO_UUID_LEN, KATAPULT_HASH_SEED);
    std::copy(reinterpret_cast<uint8_t*>(&hash), reinterpret_cast<uint8_t*>(&hash) + CANBUS_UUID_LEN, fast_hash_uid.begin());

    return fast_hash_uid;
}

uint Base_module::Send_CAN_message(App_messages::Base_message &message) {
    if (Singleton_instance()) {
        return Singleton_instance()->can_thread->Send((message));
    } else {
        return 0;
    }
}

uint Base_module::Send_CAN_message(CAN::Message const &message) {
    if (Singleton_instance()) {
        return Singleton_instance()->can_thread->Send((message));
    } else {
        return 0;
    }
}

std::optional<float> Base_module::Version_voltage() const{
    bool lock = adc_mutex->Lock(0);
    if (!lock) {
        Logger::Warning("HW version ADC mutex lock failed");
        return std::nullopt;
    }
    float version_voltage = version_voltage_channel->Read_voltage();
    adc_mutex->Unlock();
    return version_voltage;
}


void Base_module::Connect_to_cli(CLI_service& cli) const{
    Logger::Notice("Connecting to cli");
    
    if(base_module_cli){
        this->base_module_cli->Connect_to_cli(cli);
    } 
    
    this->Setup_cli(cli);
}