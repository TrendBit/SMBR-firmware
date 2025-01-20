#include "mini_oled.hpp"

Mini_OLED::Mini_OLED(uint32_t data_update_rate_s):
    Component(Codes::Component::Mini_OLED),
    Message_receiver(Codes::Component::Mini_OLED),
    data_update_rate_s(data_update_rate_s),
    lvgl_thread(new Mini_display_thread())
{
    auto update_data_lambda = [this, data_update_rate_s](){
        Logger::Print("Updating data");
        Application_message sid_request(Codes::Module::Core_module, Codes::Instance::Exclusive, Codes::Message_type::Core_SID_request);
        Application_message ip_request(Codes::Module::Core_module, Codes::Instance::Exclusive, Codes::Message_type::Core_IP_request);
        Application_message hostname_request(Codes::Module::Core_module, Codes::Instance::Exclusive, Codes::Message_type::Core_hostname_request);
        Application_message serial_request(Codes::Module::Core_module, Codes::Instance::Exclusive, Codes::Message_type::Core_serial_request);

        Send_CAN_message(sid_request);
        Send_CAN_message(ip_request);
        Send_CAN_message(hostname_request);
        Send_CAN_message(serial_request);

        update_data->Execute(data_update_rate_s * 1000);
    };

    update_data = new rtos::Delayed_execution(update_data_lambda, data_update_rate_s * 1000, true);
}

bool Mini_OLED::Receive(CAN::Message message){
    UNUSED(message);
    return true;
}

bool Mini_OLED::Receive(Application_message message){
    switch (message.Message_type()) {
        case Codes::Message_type::Core_SID_response: {
            App_messages::Core::SID_response sid_response;

            if (!sid_response.Interpret_data(message.data)) {
                Logger::Print("SID_response interpretation failed");
                return false;
            }

            Logger::Print(emio::format("Received SID: 0x{:04x}", sid_response.sid));
            lvgl_thread->Update_SID(sid_response.sid);
            return true;
        }

        case Codes::Message_type::Core_serial_response: {
            App_messages::Core::Serial_response serial_response;

            if (!serial_response.Interpret_data(message.data)) {
                Logger::Print("Serial_response interpretation failed");
                return false;
            }

            Logger::Print(emio::format("Received serial: {}", serial_response.serial_number));
            lvgl_thread->Update_serial(serial_response.serial_number);
            return true;
        }

        case Codes::Message_type::Core_hostname_response: {
            App_messages::Core::Hostname_response hostname_response;

            if (!hostname_response.Interpret_data(message.data)) {
                Logger::Print("Hostname_response interpretation failed");
                return false;
            }

            Logger::Print(emio::format("Received hostname: {}", hostname_response.hostname));
            lvgl_thread->Update_hostname(hostname_response.hostname);
            return true;
        }

        case Codes::Message_type::Core_IP_response: {
            App_messages::Core::IP_address_response ip_response;

            if (!ip_response.Interpret_data(message.data)) {
                Logger::Print("IP_response interpretation failed");
                return false;
            }

            Logger::Print(emio::format("Received IP: {}.{}.{}.{}", ip_response.IP_address[0], ip_response.IP_address[1], ip_response.IP_address[2], ip_response.IP_address[3]));
            lvgl_thread->Update_ip(ip_response.IP_address);
            return true;
        }

        default:
            return false;
    }
}
