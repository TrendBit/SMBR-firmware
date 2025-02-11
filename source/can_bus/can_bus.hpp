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

#include "queue.hpp"

#include "etl/unordered_map.h"
#include "etl/queue.h"

#include "hal/irq/irq_capable.hpp"
#include "can_message.hpp"

#include "logger.hpp"
#include "emio/emio.hpp"

/**
 * @brief Watch out: This extern can bite you
 */
extern "C" {
#include "can2040.h"
}
#include "RP2040.h"

#define UNUSED(x) (void)(x)

namespace fra = cpp_freertos;

/**
 * @brief
 */
namespace CAN {

/**
 * @brief   Class representing can bus peripheral implemented in PIO unit using can2040 library
 *          This class is IRQ capable, support IRQ at receiving, transmitting and error detection
 *          There could be only two instance of this class, one for each PIO unit (4 state machines are used)
 *          CAN2.0B data frames are supported (11 bit and 29 bit identifiers)
 */
class Bus : public IRQ_capable {
private:
    /**
     * @brief Number of pio unit used for this bus
     */
    uint pio_number;

    /**
     * @brief Frequency of system clock in Hz
     */
    uint32_t sys_clock;

    /**
     * @brief Internal can2040 handler structure, passed into library functions
     */
    struct can2040 handler;

    /**
     * @brief Map of can bus peripheral instances used to map callbacks back to class instances
     */
    inline static etl::unordered_map<uint, CAN::Bus *, 2> instances;

    /**
     * @brief   Last received message, overwritten by new one when new message is received
     *          Message caching needs to be implemented in higher level
     */
    std::optional<Message> last_received;

    /**
     * @brief   Queue containing raw can2040 messages received from can bus
     *          Queue is ISR safe, implemented as part of FreeRTOS sync routines
     */
    fra::Queue * rx_queue;

public:
    /**
     * @brief Supported IRQ types for Can bus peripheral
     */
    enum class IRQ_type: uint8_t{
        Any,
        RX,
        TX,
        Error
    };

    /**
     * @brief   Internal class callback executed by global level callback
     *
     * @param notify    Type of event triggering callback which is translated into IRQ_type
     * @param msg       Message received or transmitted in case of RX event, pointer is only valid during callback
     */
    void Callback(uint32_t notify, struct can2040_msg *msg);

    /**
     * @brief   Global level callback called by can2040 library, inside is determined instance of class based on pio number and instance map
     *          Class level callback is then called and event is processed
     *
     * @param cd        Pointer to can2040 handler structure
     * @param notify    Type of event triggering callback which is translated into IRQ_type
     * @param msg       Message corresponding to event
     */
    static void Callback_handler(struct can2040 *cd, uint32_t notify, struct can2040_msg *msg);

    /**
     * @brief   Construct a new CAN Bus object, based on can2040 library
     *          Uses whole pio unit for peripheral (4 state machines)
     *
     * @param gpio_rx   CAN RX pin
     * @param gpio_tx   CAN TX pin
     * @param bitrate   Bitrate of CAN Bus
     * @param pio_num   Number of pio unit used for this bus, default is 0
     */
    Bus(unsigned int gpio_rx, unsigned int gpio_tx, uint bitrate, uint pio_num = 0);

    /**
     * @brief   Transmit message over can bus
     *          Should be checked is transmission is available before calling this function, otherwise message can be dropped
     *
     * @param msg       "Legacy" message structure used by can2040 library
     * @return true     Message was transmitted
     * @return false    Message was not transmitted
     */
    bool Transmit(struct can2040_msg *msg);

    /**
     * @brief   Transmit message over can bus
     *          Should be checked is transmission is available before calling this function, otherwise message can be dropped
     *
     * @param message   Message to be transmitted
     * @return true     Message was transmitted
     * @return false    Message was not transmitted
     */
    bool Transmit(Message const &message);

    /**
     * @brief   Check if can bus is available for transmitting new message
     *          Can2040 CAN bus peripheral has internal buffer for around 8 messages
     *          If Internal buffer is full, this function will return false
     *
     * @return true     Can bus is available for transmitting
     * @return false    Can bus is not available for transmitting
     */
    bool Transmit_available();

    /**
     * @brief Returns last received message on CAN bus
     *
     * @return std::optional<can2040_msg>   Last received message if any was received, otherwise empty optional
     */
    std::optional<can2040_msg> Receive();

    /**
     * @brief   Returns number of messages in receive queue waiting for processing
     *
     * @return uint8_t  Number of messages in receive queue
     */
    uint8_t Received_queue_size();

private:
    /**
     * @brief   Enable IRQ for PIO unit used by this CAN bus peripheral
     *
     * @tparam T_id Pico-lib internal IRQ channel number, must be unique for each IRQ
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
     * @brief  Pico-SDK Callback to handle IRQs when happens and pass it to can2040 library
     */
    void Handle_PIO_IRQ();
};
};
