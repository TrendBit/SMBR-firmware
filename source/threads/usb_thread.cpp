#include "usb_thread.hpp"

USB_thread::USB_thread()
    : Thread("usb_thread", 1000, 1){
    Start();
};

void USB_thread::Run(){
    tusb_init();

    while (true) {
        tud_task();
        Yield();
    }
};
