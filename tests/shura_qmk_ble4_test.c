// SPDX-License-Identifier: GPL-2.0-or-later
#include "shura_ble4_test_board.h"
#include "bluetooth.h"
#include "connection.h"

#include <assert.h>
#include <string.h>

uint32_t test_ms;
static const uint8_t *rx;
static size_t rx_left;
static struct { uint8_t bytes[9]; size_t length; uint32_t ms; } sent[32];
static size_t sent_count;
static bool busy, wake_low;
static unsigned wake_pulses;
static uint32_t pulse_end;

bool test_line_read(unsigned line) {
    assert(line == SHURA_BLE_TO_MCU_PIN);
    return busy;
}

void test_line_write(unsigned line, bool high) {
    if (line != SHURA_BLE_FROM_MCU_PIN) return;
    if (high && wake_low) {
        wake_pulses++;
        pulse_end = test_ms;
    }
    wake_low = !high;
}

int test_uart_read(void) {
    if (!rx_left) return -1;
    rx_left--;
    return *rx++;
}

void test_uart_write(const uint8_t *bytes, size_t length) {
    assert(sent_count < ARRAY_SIZE(sent) && length && length <= sizeof(sent[0].bytes));
    sent[sent_count].ms = test_ms;
    sent[sent_count].length = length;
    memcpy(sent[sent_count++].bytes, bytes, length);
}

void connection_set_host_noeeprom(connection_host_t host) { (void)host; }

static void tick(void) {
    test_ms += 10;  // Enough time for the module's inter-message gap.
    bluetooth_task();
}

static void advance_to(uint32_t deadline) {
    while (test_ms < deadline) {
        test_ms++;
        bluetooth_task();
    }
}

static void reply(const uint8_t *bytes, size_t length) {
    rx = bytes;
    rx_left = length;
    tick();
    assert(!rx_left);
}

static void key(uint8_t usage) {
    report_keyboard_t report = {.keys = {usage}};
    bluetooth_send_keyboard(&report);
}

static void expect_key(size_t index, uint8_t usage) {
    const uint8_t frame[9] = {0xA1, 0, 0, usage};
    assert(index < sent_count && sent[index].length == sizeof(frame));
    assert(memcmp(sent[index].bytes, frame, sizeof(frame)) == 0);
}

static void startup(uint8_t status) {
    bluetooth_init();
    tick();
    reply((uint8_t[]){0xBE, 'v', '0', '1'}, 4);
    for (uint8_t slot = 1; slot <= 4; slot++) reply((uint8_t[]){0xBA, slot, 0}, 3);
    reply((uint8_t[]){0xBB, 0, 0, 0, 0}, 5);
    reply((uint8_t[]){0xBC, 1, status}, 3);
}

static void passkey(void) {
    startup(0x01);  // Pairing; no connected status yet.
    sent_count = 0;

    report_mouse_t mouse = {.buttons = 1, .x = 5};
    bluetooth_send_mouse(&mouse);
    bluetooth_send_consumer(AUDIO_VOL_UP);
    const uint8_t digits_and_enter[] = {KC_7, KC_7, KC_4, KC_7, KC_4, KC_4, KC_ENTER};
    for (size_t i = 0; i < ARRAY_SIZE(digits_and_enter); i++) {
        key(digits_and_enter[i]);
        key(0);
    }
    // A further pairing notification must preserve queued digits. Allow idle ticks
    // after the keys drain so an incorrectly ungated mouse/media report is observable.
    reply((uint8_t[]){0xBC, 1, 0x10}, 3);
    for (int i = 0; i < 20; i++) tick();
    assert(sent_count == 2 * ARRAY_SIZE(digits_and_enter));
    for (size_t i = 0; i < ARRAY_SIZE(digits_and_enter); i++) {
        expect_key(2 * i, digits_and_enter[i]);
        expect_key(2 * i + 1, 0);
    }

    sent_count = 0;
    key(KC_7);
    key(0);  // Both still pending when the module announces a connection.
    reply((uint8_t[]){0xBC, 1, 0x02}, 3);
    for (int i = 0; i < 20; i++) tick();
    for (size_t i = 0; i < sent_count; i++) assert(sent[i].bytes[0] != 0xA1);

    sent_count = 0;
    key(KC_A);
    tick();
    assert(sent_count == 1);
    expect_key(0, KC_A);
}

static void expect_frame(size_t index, const uint8_t *frame, size_t length) {
    assert(index < sent_count && sent[index].length == length);
    assert(memcmp(sent[index].bytes, frame, length) == 0);
}

static void slot_event(uint16_t code, bool down) {
    keyrecord_t record = {.event.pressed = down};
    assert(!ble_process_record(code, &record));
}

// Complete a possible A7 transaction without changing the link state.
static void ack(void) {
    reply((uint8_t[]){0xBD, 0}, 2);
}

static void expect_powerdown_at(uint32_t deadline) {
    sent_count = 0;
    advance_to(deadline - 1);
    assert(!ble4_radio_asleep() && sent_count == 0);
    advance_to(deadline + 10);  // Allow the TX scheduler and pulse-settling delay.
    assert(ble4_radio_asleep() && sent_count == 1);
    expect_frame(0, (uint8_t[]){0xAF, 0x5A, 0x01, 0x03, 0x05}, 5);
}

// False must not merely mean that we have not sampled PB10 three times yet.
static void expect_sleep_blocked(void) {
    for (int i = 0; i < 4; i++) assert(!ble4_can_sleep());
}

static void connected_idle(void) {
    startup(0x02);
    sent_count = 0;
    unsigned pulses = wake_pulses;
    // Traffic shortly before 2 s needs no pulse and does not restart the pulse timer.
    uint32_t last_pulse = pulse_end;
    advance_to(last_pulse + 1990);
    key(KC_A);
    bluetooth_task();
    assert(sent_count == 1 && wake_pulses == pulses);
    advance_to(last_pulse + 2020);
    key(0);
    bluetooth_task();
    assert(sent_count == 2 && wake_pulses == pulses + 1);
    sent_count = 0;
    pulses = wake_pulses;
    advance_to(test_ms + 125000);
    assert(sent_count == 0 && wake_pulses == pulses && !ble4_radio_asleep());

    // PB10 must be *consecutively* low. A high sample breaks a streak.
    busy = true;
    assert(!ble4_can_sleep());
    busy = false;
    assert(!ble4_can_sleep());
    busy = true;
    assert(!ble4_can_sleep());
    busy = false;
    assert(!ble4_can_sleep());
    assert(!ble4_can_sleep());
    assert(ble4_can_sleep());

    key(KC_A);
    expect_sleep_blocked();
    tick();
    assert(sent_count == 1 && wake_pulses == pulses + 1);
    expect_key(0, KC_A);
    expect_sleep_blocked();  // TX just started.
    tick();
    for (int i = 0; i < 3; i++) (void)ble4_can_sleep();
    assert(ble4_can_sleep());
    sent_count = 0;
    pulses = wake_pulses;
    ble_sleep();
    assert(sent_count == 0 && !ble4_radio_asleep());
    ble_wake();
    tick();
    assert(sent_count == 0 && wake_pulses == pulses + 1 && !ble4_radio_asleep());
}

static void connected_handoff(void) {
    startup(0x02);  // Slot 1 is connected when the user taps slot 3.
    sent_count = 0;
    slot_event(BLE_SLOT1 + 2, true);
    slot_event(BLE_SLOT1 + 2, false);
    tick();
    assert(sent_count == 1);
    expect_frame(0, (uint8_t[]){0xA7, 3}, 2);
    ack();  // Give the scheduler a chance to send HID before any BC arrives.
    key(KC_7);
    tick();
    assert(sent_count == 1);

    reply((uint8_t[]){0xBC, 1, 0x00}, 3);
    tick();
    assert(sent_count == 1);  // No reconnect to slot 1.
    reply((uint8_t[]){0xBC, 1, 0x02}, 3);
    key(0);
    tick();
    assert(sent_count == 1);  // A stale connection cannot authorize keyboard reports.

    reply((uint8_t[]){0xBC, 3, 0x02}, 3);
    tick();  // Let any pending keyboard state reach the selected host.
    size_t before_key = sent_count;
    key(KC_A);
    tick();
    assert(sent_count == before_key + 1);
    expect_key(before_key, KC_A);

    reply((uint8_t[]){0xBC, 3, 0x00}, 3);
    tick();
    assert(sent_count == before_key + 2);
    expect_frame(before_key + 1, (uint8_t[]){0xA7, 3}, 2);
}

static void disconnected_wake(bool use_slot) {
    startup(0x02);
    uint32_t disconnect = test_ms + 10;  // Timestamp of the following BC handler.
    reply((uint8_t[]){0xBC, 1, 0}, 3);
    ack();
    advance_to(disconnect + 20000);
    reply((uint8_t[]){0xBC, 1, 0}, 3);  // Duplicate near expiry must not buy another 25 s.
    ack();
    expect_powerdown_at(disconnect + 25000);
    unsigned pulses = wake_pulses;

    // Delayed BC and BF packets and all forms of HID traffic are inert after AF.
    reply((uint8_t[]){0xBC, 1, 0x02, 0xBF, 0, 0, 0, 0, 0, 0, 0, 0}, 12);
    key(KC_A);
    report_mouse_t mouse = {.buttons = 1, .x = 5};
    bluetooth_send_mouse(&mouse);
    bluetooth_send_consumer(AUDIO_VOL_UP);
    advance_to(test_ms + 125000);
    assert(sent_count == 1 && wake_pulses == pulses && ble4_radio_asleep());
    for (int i = 0; i < 3; i++) (void)ble4_can_sleep();
    assert(ble4_can_sleep());

    sent_count = 0;
    if (use_slot) {
        slot_event(BLE_SLOT1 + 2, true);
        slot_event(BLE_SLOT1 + 2, false);
    } else {
        ble_sleep();
        assert(sent_count == 0 && ble4_radio_asleep());
        ble_wake();
    }
    advance_to(test_ms + 30);
    assert(wake_pulses == pulses + 1 && sent_count == 1);
    expect_frame(0, (uint8_t[]){0xAC, 0x55}, 2);
    assert(sent[0].ms >= pulse_end + 12);
    expect_sleep_blocked();  // AC still awaits a reply.
    if (use_slot) {
        reply((uint8_t[]){0xBC, 1, 0}, 3);
        tick();
        assert(sent_count == 2);
        expect_frame(1, (uint8_t[]){0xA7, 3}, 2);  // Not overwritten by BC's reconnect.
    }
    reply((uint8_t[]){0xBC, 3, 0x02}, 3);
    advance_to(test_ms + 100);
    assert(sent_count == (use_slot ? 2u : 1u));  // No replay of the sleeping HID traffic.
    key(KC_7);
    tick();
    expect_key(sent_count - 1, KC_7);
}

static void action_window(bool pair, uint8_t late_status) {
    // Start already in the state that the action's status reply will report: only
    // a real transition should shorten the explicit 120 s window to 25 s.
    uint8_t status = pair ? 0x01 : 0x00;
    startup(status);
    ack();
    advance_to(test_ms + 10000);
    sent_count = 0;
    uint16_t code = pair ? BLE_PAIR : BLE_SLOT1;
    slot_event(code, true);
    if (pair) advance_to(test_ms + 3000);
    uint32_t action = test_ms;
    if (pair) ble_housekeeping();
    slot_event(code, false);
    bluetooth_task();
    tick();
    assert(sent_count == 1);
    expect_frame(0, (uint8_t[]){pair ? 0xA6 : 0xA7, 1}, 2);
    reply((uint8_t[]){0xBC, 1, status}, 3);
    ack();
    advance_to(action + 90000);
    assert(!ble4_radio_asleep());
    expect_sleep_blocked();
    uint32_t late = test_ms + 10;
    reply((uint8_t[]){0xBC, 1, late_status}, 3);
    ack();
    expect_powerdown_at(late_status == status ? action + 120000 : late + 25000);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    if (strcmp(argv[1], "passkey") == 0) passkey();
    else if (strcmp(argv[1], "connected-idle") == 0) connected_idle();
    else if (strcmp(argv[1], "connected-handoff") == 0) connected_handoff();
    else if (strcmp(argv[1], "slot-wake") == 0) disconnected_wake(true);
    else if (strcmp(argv[1], "mcu-wake") == 0) disconnected_wake(false);
    else if (strcmp(argv[1], "slot-window") == 0) action_window(false, 0x00);
    else if (strcmp(argv[1], "pair-window") == 0) action_window(true, 0x01);
    else if (strcmp(argv[1], "link-change") == 0) action_window(true, 0x00);
    else return 1;
    return 0;
}
