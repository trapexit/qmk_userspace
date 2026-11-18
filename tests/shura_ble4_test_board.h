// SPDX-License-Identifier: GPL-2.0-or-later
// Host stand-ins for the board/ChibiOS surface of ble4.c. HID types come from QMK.
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct { struct { bool pressed; } event; } keyrecord_t;
enum { BLE_SLOT1 = 0x7000, BLE_PAIR = BLE_SLOT1 + 4, BLE_USB };

typedef int msg_t;
typedef struct { unsigned speed; } SerialConfig;
extern uint32_t test_ms;
int test_uart_read(void);
void test_uart_write(const uint8_t *bytes, size_t length);
enum { SHURA_BLE_FROM_MCU_PIN, SHURA_BLE_TO_MCU_PIN, SHURA_BLE_RESET_PIN, SHURA_BLE_TX_PIN };
enum { PAL_LOW, PAL_HIGH };
void test_line_write(unsigned line, bool high);
bool test_line_read(unsigned line);

bool ble_process_record(uint16_t keycode, keyrecord_t *record);
void ble_housekeeping(void);
void ble_sleep(void);
void ble_wake(void);
bool ble4_radio_asleep(void);
bool ble4_can_sleep(void);

#define sdGetTimeout(sd, timeout) test_uart_read()
#define sdWrite(sd, bytes, length) test_uart_write(bytes, length)
#define sdStart(sd, config) ((void)(config))
#define sdStop(sd) ((void)0)
#define timer_read32() (test_ms)
#define timer_read() ((uint16_t)test_ms)
#define timer_elapsed32(start) (test_ms - (start))
#define timer_elapsed(start) ((uint16_t)(test_ms - (start)))
#define wait_ms(ms) ((void)(test_ms += (ms)))
#define wait_us(us) ((void)0)
#define palClearLine(line) test_line_write(line, false)
#define palSetLine(line) test_line_write(line, true)
#define palReadLine(line) test_line_read(line)
#define palSetLineMode(line, mode) ((void)0)
#define palToggleLine(line) ((void)0)
#define led_update_ports(state) ((void)0)
