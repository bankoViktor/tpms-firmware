/**
 ********************************************************************************
 * @file    uhf_receiver.h
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    07.01.2025
 * @brief   Header file of the UHF receiver.
 ********************************************************************************
 */

// CC1101 has the following connections:
//                               +---+---+
//                           GND | 1 | 2 | VCC
//          D15 <----       GDO0 | 3 | 4 | CSN   ----> D5 (VSPI)
//   (VSPI) D18 <----        SCK | 5 | 6 | MOSI  ----> D23 (VSPI)
//   (VSPI) D19 <----  MISO/GDO1 | 7 | 8 | GDO2
//                               +---+---+

#ifndef UHF_RECEIVER__H
#define UHF_RECEIVER__H

#include <stdio.h>
#include <RadioLib.h>

/// @brief UHF receiver. It is a singleton.
class UhfReceiver
{
    using received_callback_f = std::function<void()>;

private:
    static constexpr uint32_t PIN_CS = 5;
    static constexpr uint32_t PIN_IRQ = 15;
    static constexpr size_t PACKET_LENGTH = 18;
    static constexpr uint8_t SYNC_WORD_HIGH = 0xA9;
    static constexpr uint8_t SYNC_WORD_LOW = 0x55;

    /// @brief Instance of the radio module.
    CC1101 m_module;

    /// @brief Flag to indicate that a packet was received.
    static volatile bool s_xReceivedFlag;

    /// @brief Callback for data received event.
    received_callback_f m_receivedCallback;

    /// @brief Message counter.
    uint32_t m_dwMsgCount;

    /// @brief Radio module interrupt callback function.
    static void packetReceivedCallback();

    /// @brief Process received packet.
    void processReceivedPacket();

public:
    /// @brief Default constructor.
    UhfReceiver();

    /// @brief Initialize instance.
    void begin();

    /// @brief Loop handle.
    void loopHandle();

    /// @brief Set callback for packet received event.
    /// @param func
    void setReceivedCallback(received_callback_f func);
};

#endif
