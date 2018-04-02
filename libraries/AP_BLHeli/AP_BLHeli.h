/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
  implementation of MSP and BLHeli-4way protocols for pass-through ESC
  calibration and firmware update

  With thanks to betaflight for a great reference implementation
 */

#pragma once

#include <AP_Common/AP_Common.h>
#include <AP_HAL/AP_HAL.h>

#if HAL_SUPPORT_RCOUT_SERIAL

#include <AP_Param/AP_Param.h>
#include "msp_protocol.h"
#include "blheli_4way_protocol.h"

class AP_BLHeli {

public:
    AP_BLHeli();
    
    void update(void);
    void update_telemetry(void);
    bool process_input(uint8_t b);

    static const struct AP_Param::GroupInfo var_info[];
    
private:

    // mask of channels to use for BLHeli protocol
    AP_Int32 channel_mask;
    AP_Int8 channel_auto;
    AP_Int8 run_test;
    AP_Int16 timeout_sec;
    
    enum mspState {
        MSP_IDLE=0,
        MSP_HEADER_START,
        MSP_HEADER_M,
        MSP_HEADER_ARROW,
        MSP_HEADER_SIZE,
        MSP_HEADER_CMD,
        MSP_COMMAND_RECEIVED
    };

    enum mspPacketType {
        MSP_PACKET_COMMAND,
        MSP_PACKET_REPLY
    };

    enum escProtocol {
        PROTOCOL_SIMONK = 0,
        PROTOCOL_BLHELI = 1,
        PROTOCOL_KISS = 2,
        PROTOCOL_KISSALL = 3,
        PROTOCOL_CASTLE = 4,
        PROTOCOL_MAX = 5,
        PROTOCOL_NONE = 0xfe,
        PROTOCOL_4WAY = 0xff
    };

    enum motorPwmProtocol {
        PWM_TYPE_STANDARD = 0,
        PWM_TYPE_ONESHOT125,
        PWM_TYPE_ONESHOT42,
        PWM_TYPE_MULTISHOT,
        PWM_TYPE_BRUSHED,
        PWM_TYPE_DSHOT150,
        PWM_TYPE_DSHOT300,
        PWM_TYPE_DSHOT600,
        PWM_TYPE_DSHOT1200,
        PWM_TYPE_PROSHOT1000,
    };

    enum MSPFeatures {
        FEATURE_RX_PPM = 1 << 0,
        FEATURE_INFLIGHT_ACC_CAL = 1 << 2,
        FEATURE_RX_SERIAL = 1 << 3,
        FEATURE_MOTOR_STOP = 1 << 4,
        FEATURE_SERVO_TILT = 1 << 5,
        FEATURE_SOFTSERIAL = 1 << 6,
        FEATURE_GPS = 1 << 7,
        FEATURE_RANGEFINDER = 1 << 9,
        FEATURE_TELEMETRY = 1 << 10,
        FEATURE_3D = 1 << 12,
        FEATURE_RX_PARALLEL_PWM = 1 << 13,
        FEATURE_RX_MSP = 1 << 14,
        FEATURE_RSSI_ADC = 1 << 15,
        FEATURE_LED_STRIP = 1 << 16,
        FEATURE_DASHBOARD = 1 << 17,
        FEATURE_OSD = 1 << 18,
        FEATURE_CHANNEL_FORWARDING = 1 << 20,
        FEATURE_TRANSPONDER = 1 << 21,
        FEATURE_AIRMODE = 1 << 22,
        FEATURE_RX_SPI = 1 << 25,
        FEATURE_SOFTSPI = 1 << 26,
        FEATURE_ESC_SENSOR = 1 << 27,
        FEATURE_ANTI_GRAVITY = 1 << 28,
        FEATURE_DYNAMIC_FILTER = 1 << 29,
    };


    /*
      state of MSP command processing
    */
    struct {
        enum mspState state;
        enum mspPacketType packetType;
        uint8_t offset;
        uint8_t dataSize;
        uint8_t checksum;
        uint8_t buf[192];
        uint8_t cmdMSP;
        enum escProtocol escMode;
        uint8_t portIndex;
    } msp;

    enum blheliState {
        BLHELI_IDLE=0,
        BLHELI_HEADER_START,
        BLHELI_HEADER_CMD,
        BLHELI_HEADER_ADDR_LOW,
        BLHELI_HEADER_ADDR_HIGH,
        BLHELI_HEADER_LEN,
        BLHELI_CRC1,
        BLHELI_CRC2,
        BLHELI_COMMAND_RECEIVED
    };

    /*
      state of blheli 4way protocol handling
    */
    struct {
        enum blheliState state;
        uint8_t command;
        uint16_t address;
        uint16_t param_len;
        uint16_t offset;
        uint8_t buf[256+3+8];
        uint8_t crc1;
        uint16_t crc;
        uint8_t interface_mode;
        uint8_t deviceInfo[4];
        uint8_t chan;
        uint8_t ack;
    } blheli;

    AP_HAL::UARTDriver *uart;
    AP_HAL::UARTDriver *debug_uart;
    
    static const uint8_t max_motors = 8;
    uint8_t num_motors;

    // have we initialised the interface?
    bool initialised;

    // last valid packet timestamp
    uint32_t last_valid_ms;

    // when did we start the serial ESC output?
    uint32_t serial_start_ms;

    // have we disabled motor outputs?
    bool motors_disabled;

    // mapping from BLHeli motor numbers to RC output channels
    uint8_t motor_map[max_motors];

    bool msp_process_byte(uint8_t c);
    void blheli_crc_update(uint8_t c);
    bool blheli_4way_process_byte(uint8_t c);
    void msp_send_reply(uint8_t cmd, const uint8_t *buf, uint8_t len);
    void putU16(uint8_t *b, uint16_t v);
    uint16_t getU16(const uint8_t *b);
    void putU32(uint8_t *b, uint32_t v);
    void putU16_BE(uint8_t *b, uint16_t v);
    void msp_process_command(void);
    void blheli_send_reply(const uint8_t *buf, uint16_t len);
    uint16_t BL_CRC(const uint8_t *buf, uint16_t len);
    bool isMcuConnected(void);
    void setDisconnected(void);
    bool BL_SendBuf(const uint8_t *buf, uint16_t len);
    bool BL_ReadBuf(uint8_t *buf, uint16_t len);
    uint8_t BL_GetACK(uint16_t timeout_ms=2);
    bool BL_SendCMDSetAddress();
    bool BL_ReadA(uint8_t cmd, uint8_t *buf, uint16_t n);
    bool BL_ConnectEx(void);
    bool BL_SendCMDKeepAlive(void);
    bool BL_PageErase(void);
    void BL_SendCMDRunRestartBootloader(void);
    uint8_t BL_SendCMDSetBuffer(const uint8_t *buf, uint16_t nbytes);
    bool BL_WriteA(uint8_t cmd, const uint8_t *buf, uint16_t nbytes, uint32_t timeout);
    uint8_t BL_WriteFlash(const uint8_t *buf, uint16_t n);
    bool BL_VerifyFlash(const uint8_t *buf, uint16_t n);
    void blheli_process_command(void);
    void run_connection_test(uint8_t chan);

    // protocol handler hook
    bool protocol_handler(uint8_t , AP_HAL::UARTDriver *);
};


// start of 12 byte CPU ID
#ifndef UDID_START
#define UDID_START	0x1FFF7A10
#endif

#endif // HAL_SUPPORT_RCOUT_SERIAL
