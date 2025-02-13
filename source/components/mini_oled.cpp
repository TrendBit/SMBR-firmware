#include "mini_oled.hpp"

Mini_OLED::Mini_OLED(Bottle_temperature * const bottle_temp_sensor, uint32_t data_update_rate_s) :
    Component(Codes::Component::Mini_OLED),
    Message_receiver(Codes::Component::Mini_OLED),
    data_update_rate_s(data_update_rate_s),
    lvgl_thread(new Mini_display_thread()),
    bottle_temp_sensor(bottle_temp_sensor)
{

    auto update_data_lambda = [this, data_update_rate_s](){
          Logger::Print("Updating data");
          Application_message sid_request(Codes::Module::Core_module, Codes::Instance::Exclusive, Codes::Message_type::Core_SID_request);
          Application_message ip_request(Codes::Module::Core_module, Codes::Instance::Exclusive, Codes::Message_type::Core_IP_request);
          Application_message hostname_request(Codes::Module::Core_module, Codes::Instance::Exclusive, Codes::Message_type::Core_hostname_request);
          Application_message serial_request(Codes::Module::Core_module, Codes::Instance::Exclusive, Codes::Message_type::Core_serial_request);
          Application_message target_temp_request(Codes::Module::Control_module, Codes::Instance::Exclusive, Codes::Message_type::Heater_get_target_temperature_request);
          Application_message plate_temp_request(Codes::Module::Control_module, Codes::Instance::Exclusive, Codes::Message_type::Heater_get_plate_temperature_request);

          Send_CAN_message(sid_request);
          Send_CAN_message(ip_request);
          Send_CAN_message(hostname_request);
          Send_CAN_message(serial_request);
          Send_CAN_message(target_temp_request);
          Send_CAN_message(plate_temp_request);

          //lvgl_thread->);

          update_data->Execute(data_update_rate_s * 1000);
      };

    Message_router::Register_bypass(Codes::Message_type::Core_SID_response, Codes::Component::Mini_OLED);
    Message_router::Register_bypass(Codes::Message_type::Core_IP_response, Codes::Component::Mini_OLED);
    Message_router::Register_bypass(Codes::Message_type::Core_hostname_response, Codes::Component::Mini_OLED);
    Message_router::Register_bypass(Codes::Message_type::Core_serial_response, Codes::Component::Mini_OLED);
    Message_router::Register_bypass(Codes::Message_type::Heater_get_target_temperature_response, Codes::Component::Mini_OLED);
    Message_router::Register_bypass(Codes::Message_type::Heater_get_plate_temperature_response, Codes::Component::Mini_OLED);
    Message_router::Register_bypass(Codes::Message_type::Bottle_temperature_response, Codes::Component::Mini_OLED);

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

        case Codes::Message_type::Mini_OLED_clear_custom_text: {
            App_messages::Mini_OLED::Clear_custom_text clear_custom_text;

            if (!clear_custom_text.Interpret_data(message.data)) {
                Logger::Print("Clear_custom_text interpretation failed");
                return false;
            }

            Logger::Print("Clearing custom text");
            lvgl_thread->Clear_custom_text();
            return true;
        }

        case Codes::Message_type::Mini_OLED_print_custom_text: {
            App_messages::Mini_OLED::Print_custom_text print_custom_text;

            if (!print_custom_text.Interpret_data(message.data)) {
                Logger::Print("Print_custom_text interpretation failed");
                return false;
            }

            Logger::Print(emio::format("Printing custom text: {}", print_custom_text.text));

            if (not print_custom_text.text.empty()) {
                lvgl_thread->Print_custom_text(print_custom_text.text);
            }

            return true;
        }

        case Codes::Message_type::Heater_get_target_temperature_response: {
            App_messages::Heater::Get_target_temperature_response target_temperature_response;

            if (!target_temperature_response.Interpret_data(message.data)) {
                Logger::Print("Heater_get_target_temperature_response interpretation failed");
                return false;
            }

            Logger::Print(emio::format("Received target temperature: {:05.2f}˚C", target_temperature_response.temperature));
            lvgl_thread->Set_target_temperature(target_temperature_response.temperature);
            return true;
        }

        case Codes::Message_type::Heater_get_plate_temperature_response: {
            App_messages::Heater::Get_plate_temperature_response plate_temperature_response;

            if (!plate_temperature_response.Interpret_data(message.data)) {
                Logger::Print("Heater_get_plate_temperature_response interpretation failed");
                return false;
            }

            Logger::Print(emio::format("Received plate temperature: {:05.2f}˚C", plate_temperature_response.temperature));
            lvgl_thread->Set_plate_temperature(plate_temperature_response.temperature);
            lvgl_thread->Set_bottle_temperature(bottle_temp_sensor->Temperature());
            lvgl_thread->Update_temps();
            return true;
        }

        default:
            return false;
    }
} // Mini_OLED::Receive
