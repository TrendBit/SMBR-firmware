/**
 * @file canbus.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 31.05.2024
 */

#pragma once

#include "hardware/pio.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"

#include <stdint.h>
#include <vector>

#include "etl/unordered_map.h"

#include "hal/irq/irq_capable.hpp"
#include "can_message.hpp"

#include "logger.hpp"
#include "emio/emio.hpp"

extern "C" {
#include "can2040.h"
}
#include "RP2040.h"

#define UNUSED(x) (void)(x)

namespace CAN {

class Bus : public IRQ_capable {
private:
    uint pio_number;

    uint32_t sys_clock;

    struct can2040 handler;

    inline static etl::unordered_map<uint, CAN::Bus *, 2> instances;

    std::optional<Message> last_received;

public:

    enum class IRQ_type: uint8_t{
        Any,
        RX,
        TX,
        Error
    };

    void Callback(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg);

    static void Callback_handler(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg);

    Bus(unsigned int gpio_rx, unsigned int gpio_tx, uint bitrate, uint pio_num = 0);

    bool Transmit(struct can2040_msg *msg);

    bool Transmit(Message const &message);

    bool Transmit_available();

    std::optional<Message> Received();

private:

    /**
     * @brief
     *
     * @tparam T_id
     *
     * @todo
     */
    template <size_t T_id>
    void Enable_IRQ(){
        // Register IRQ handler via IRQ_handler generated function
        auto handler = Register_IRQ<T_id>(this, &Bus::Handle_PIO_IRQ);

        uint pio_irq = pio_number ? PIO1_IRQ_0 : PIO0_IRQ_0;

        irq_set_exclusive_handler(pio_irq, handler);
        irq_set_priority(pio_irq, 1);
        irq_set_enabled(pio_irq, true);
    }

    /**
     * @brief  Callback to handle IRQs when happens and pass it to can2040 library
     */
    void Handle_PIO_IRQ();
};
};
