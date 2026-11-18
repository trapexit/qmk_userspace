// Copyright 2026 Antonio SJ Musumeci (@trapexit)
// SPDX-License-Identifier: GPL-2.0-or-later
//
// The Shura mapped like the owner's TEX Shinobi (stock 3.01, KEYMAP.TEX profile 1).
// Caps Lock position is Left Ctrl; Right Alt is Page Up and the keys right of it Page
// Down. Holding the middle button: A/E Home/End, H/J/K/L arrows, N/P Page Down/Up,
// D Delete, B Backspace, W Win, F/G browser back/forward, C calculator, M mail,
// Esc USB, left/right click mouse back/forward. The Shura's own Fn layer supplies
// F1-F12 on the digits and paging on the arrows, keys the Shura lacks.
#include QMK_KEYBOARD_H

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT_all(
        KC_ESC,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_INT3,  KC_GRV,   KC_BSPC,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,
        KC_LCTL,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,  KC_ENT,
        KC_LSFT,  KC_NUBS,  KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,  KC_INT1,  KC_RSFT,  SHURA_FN1,
        KC_LCTL,  KC_LGUI,  KC_LALT,  KC_INT5,  KC_SPC,   KC_SPC,   KC_INT2,  KC_PGUP,  KC_PGDN,  KC_PGDN,  KC_PGUP,  KC_UP,    KC_PGDN,
        MS_BTN1,  SHURA_MIDDLE, MS_BTN2,  KC_LEFT,  KC_DOWN,  KC_RGHT,  BLE_PAIR
    ),
    [SHURA_FN1_LAYER] = LAYOUT_all(
        BLE_USB,  KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,   _______,  _______,  KC_DEL,
        KC_CAPS,  KC_MUTE,  KC_LGUI,  KC_END,   _______,  _______,  _______,  KC_PSCR,  KC_INS,   _______,  KC_PGUP,  _______,  _______,  _______,
        _______,  KC_HOME,  _______,  KC_DEL,   KC_WBAK,  KC_WFWD,  KC_LEFT,  KC_DOWN,  KC_UP,    KC_RGHT,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  KC_CALC,  _______,  KC_BSPC,  KC_PGDN,  KC_MAIL,  BLE_SLOT1, BLE_SLOT2, BLE_SLOT3, _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  KC_PGUP,  _______,
        MS_BTN4,  _______,  MS_BTN5,  KC_HOME,  KC_PGDN,  KC_END,   _______
    ),
    [SHURA_FN2_LAYER] = LAYOUT_all(
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______
    ),
    [SHURA_FN3_LAYER] = LAYOUT_all(
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,
        _______,  _______,  _______,  _______,  _______,  _______,  _______
    )
};
// clang-format on

// Mouse buttons 4 and 5 click once on press: TrackPoint packets only carry buttons 1-3,
// so a held back/forward button would be released by the next stick movement.
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (keycode == MS_BTN4 || keycode == MS_BTN5) {
        if (record->event.pressed) {
            report_mouse_t report = {.buttons = keycode == MS_BTN4 ? MOUSE_BTN4 : MOUSE_BTN5};
            host_mouse_send(&report);
            report.buttons = 0;
            host_mouse_send(&report);
        }
        return false;
    }
    return true;
}
