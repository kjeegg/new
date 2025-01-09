/*!
 *  \file sensorSHTC3.c
 *  \brief ?????? ??? ?????? ? ???????? SHTC3: ?????? ? ???????? ????????
 *
 *  \author Fachbereich 5 - FH Aachen (2024)
 *  \date   2024
 */

#include "sensorSHTC3.h"
#include "../lib/lcd.h"            // lcd_writeString, lcd_clear, lcd_goto...
#include "../lib/terminal.h"       // DEBUG(...) / INFO(...)
#include "../lib/util.h"           // delayMs(...) ??? ?????????????
#include "sensorData.h"            // ????????? cmd_sensorData_t, enums
#include "../adapter/rfAdapter.h"  // rfAdapter_sendSensorData(...)
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>   // sprintf
#include <string.h>  // memcpy
#include <stdbool.h>

/* 
 * ?????????? (extern) ??????? i2c_*.
 * ? ????? ??????? ??? ?????? ????????? ? i2cmaster.c ??? i2c.c
 */
extern void i2c_init(void);
extern unsigned char i2c_start(unsigned char address);
extern void i2c_stop(void);
extern unsigned char i2c_write(unsigned char data);
extern unsigned char i2c_readAck(void);
extern unsigned char i2c_readNak(void);

/*!
 * \brief I2C-????? SHTC3 (7-bit) = 0x70 (?? ????????)
 */
#define SHTC3_I2C_ADDR    0x70

// ??????? ?? ???????? (16-???):
// Sleep / Wakeup
#define SHTC3_CMD_SLEEP_MSB   0xB0
#define SHTC3_CMD_SLEEP_LSB   0x98
#define SHTC3_CMD_WAKEUP_MSB  0x35
#define SHTC3_CMD_WAKEUP_LSB  0x17

// ??????: Normal mode, T first, no clock stretching -> 0x7866
#define SHTC3_CMD_MEAS_TFIRST_NOSTRETCH_MSB  0x78
#define SHTC3_CMD_MEAS_TFIRST_NOSTRETCH_LSB  0x66

// ????? ?? ????????? (Datasheet ~12ms ? Normal Mode)
#define SHTC3_MEAS_DURATION_MS 12

//------------------------------------------------------------------------------
// ??????????????? ??????? ??? CRC-8, ???? ????? ????????? ??????????? ?????
// (??. 5.10 ? ????????: Polynomial 0x31, Init 0xFF, ??? ?????????).
//------------------------------------------------------------------------------
static uint8_t shtc3_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++)
        {
            if (crc & 0x80)
                crc = (uint8_t)((crc << 1) ^ 0x31);
            else
                crc <<= 1;
        }
    }
    return crc;
}

//------------------------------------------------------------------------------
// ????????? ??????? ??? wakeup/sleep
//------------------------------------------------------------------------------
static bool shtc3_wakeup(void)
{
    // Start+Write
    if (i2c_start((SHTC3_I2C_ADDR << 1) | 0)) {
        i2c_stop();
        return false;
    }
    // ?????????? 2 ????? (MSB, LSB)
    if (i2c_write(SHTC3_CMD_WAKEUP_MSB)) { i2c_stop(); return false; }
    if (i2c_write(SHTC3_CMD_WAKEUP_LSB)) { i2c_stop(); return false; }

    i2c_stop();
    // ?? ???????? ????? wakeup ????? ~240�s
    _delay_us(300);
    return true;
}

static bool shtc3_sleep(void)
{
    if (i2c_start((SHTC3_I2C_ADDR << 1) | 0)) {
        i2c_stop();
        return false;
    }
    if (i2c_write(SHTC3_CMD_SLEEP_MSB)) { i2c_stop(); return false; }
    if (i2c_write(SHTC3_CMD_SLEEP_LSB)) { i2c_stop(); return false; }

    i2c_stop();
    return true;
}

//------------------------------------------------------------------------------
// ???????? ??????? ?????? ????? ??????: T + RH
//------------------------------------------------------------------------------
static bool shtc3_measureRaw(uint16_t *rawT, uint16_t *rawRH)
{
    if (!rawT || !rawRH) return false;

    // ??????????
    if (!shtc3_wakeup()) {
        return false;
    }

    // ?????????? ??????? ????????? (T first, no clock stretching, Normal)
    if (i2c_start((SHTC3_I2C_ADDR << 1) | 0)) {
        i2c_stop();
        return false;
    }
    if (i2c_write(SHTC3_CMD_MEAS_TFIRST_NOSTRETCH_MSB)) { i2c_stop(); return false; }
    if (i2c_write(SHTC3_CMD_MEAS_TFIRST_NOSTRETCH_LSB)) { i2c_stop(); return false; }
    i2c_stop();

    // ???????: ~12 ms ????? ?? ?????????
    _delay_ms(SHTC3_MEAS_DURATION_MS);

    // ????????? 6 ????: T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC
    if (i2c_start((SHTC3_I2C_ADDR << 1) | 1)) {
        i2c_stop();
        return false;
    }
    uint8_t tMsb = i2c_readAck();
    uint8_t tLsb = i2c_readAck();
    uint8_t tCrc = i2c_readAck();
    uint8_t rhMsb = i2c_readAck();
    uint8_t rhLsb = i2c_readAck();
    uint8_t rhCrc = i2c_readNak();
    i2c_stop();

    // CRC-???????? ???????????
    uint8_t tempBuf[2] = { tMsb, tLsb };
    uint8_t calcT = shtc3_crc8(tempBuf, 2);
    if (calcT != tCrc) {
        DEBUG("SHTC3: Temp CRC fail!");
        shtc3_sleep();
        return false;
    }

    // CRC-???????? ?????????
    uint8_t rhBuf[2] = { rhMsb, rhLsb };
    uint8_t calcRH = shtc3_crc8(rhBuf, 2);
    if (calcRH != rhCrc) {
        DEBUG("SHTC3: RH CRC fail!");
        shtc3_sleep();
        return false;
    }

    // ???????? 16-???
    *rawT = ((uint16_t)tMsb << 8) | (tLsb);
    *rawRH = ((uint16_t)rhMsb << 8) | (rhLsb);

    // ????????? ? sleep (??? ????? ???????? �?????????�)
    shtc3_sleep();
    return true;
}

//------------------------------------------------------------------------------
// 1) ????????????? ??????? (4.3.3 / 4.3.6 Vorbereitung)
//------------------------------------------------------------------------------
void sensorSHTC3_init(void)
{
    // ?????????????? I2C (?????? ???? ??? ?? ??? ?????????? ??????)
    i2c_init();

    // ????? ??????? ??????? ????????? ??? ????????? ID (?????????????)
    // ...

    DEBUG("SHTC3 init done");
}

//------------------------------------------------------------------------------
// 2) ??????? ???????? ? ??????? ?? LCD (4.3.6 Messwerte auslesen & Display)
//------------------------------------------------------------------------------
void sensorSHTC3_measureAndDisplay(void)
{
    uint16_t rawT = 0;
    uint16_t rawRH = 0;

    bool ok = shtc3_measureRaw(&rawT, &rawRH);
    if (!ok) {
        lcd_clear();
        lcd_writeString("SHTC3 Error!");
        return;
    }

    // ??????? ?????? (?? ????????, ??. 5.11):
    //   T[�C] = -45 + 175 * (ST / 65536)
    //   RH[%] = 100 * (SRH / 65536)
    float tempC = -45.0f + 175.0f * ((float)rawT / 65535.0f);
    float humRH = 100.0f * ((float)rawRH / 65535.0f);

    // ????? ?? LCD
    lcd_clear();
    lcd_goto(0,0);
    char buf[17];

    // ??????: "T=23.4C"
    snprintf(buf, sizeof(buf), "T=%.1fC", tempC);
    lcd_writeString(buf);

    // ?? ?????? ??????: "RH=45.6%"
    lcd_goto(0,1);
    snprintf(buf, sizeof(buf), "RH=%.1f%%", humRH);
    lcd_writeString(buf);
}

//------------------------------------------------------------------------------
// 3) ??????? ???????? ? ????????? ?? ??????????? (4.3.7 Messwerte �bertragen)
//------------------------------------------------------------------------------
void sensorSHTC3_measureAndSend(void)
{
    uint16_t rawT = 0;
    uint16_t rawRH = 0;
    bool ok = shtc3_measureRaw(&rawT, &rawRH);
    if (!ok) {
        DEBUG("SHTC3 read error, skip sending");
        return;
    }

    // ????????? ? �C / %RH
    float tempC = -45.0f + 175.0f * ((float)rawT / 65535.0f);
    float humRH = 100.0f * ((float)rawRH / 65535.0f);

    // ?????????? ??? �??????� ????? rfAdapter_sendSensorData
    // 1) ???????????
    rfAdapter_sendSensorData(
        ADDRESS(1, 0),           // ???? ??????????; ????????, self ??? ?????? ????
        SENSOR_AM2320,           // ? ??? ? enum'e ??? SHTC3, ????? ???????? ??????? SENSOR_AM2320 ??? ????? ??????, ???? ????????? enum
        PARAM_TEMPERATURE_CELSIUS,
        tempC
    );

    // 2) ?????????
    rfAdapter_sendSensorData(
        ADDRESS(1, 0),
        SENSOR_AM2320,
        PARAM_HUMIDITY_PERCENT,
        humRH
    );

    DEBUG("SHTC3 T=%.1fC  RH=%.1f%% sent!", tempC, humRH);
}
