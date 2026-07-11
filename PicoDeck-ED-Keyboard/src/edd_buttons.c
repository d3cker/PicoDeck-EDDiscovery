#include "edd_buttons.h"

#include <string.h>

#include "buttons.h"

static uint32_t s_active_mask;

static const char *find_value(const char *json, size_t length,
                              const char *key) {
    size_t key_length = strlen(key);
    if (length < key_length + 3) return NULL;
    for (size_t i = 0; i + key_length + 2 < length; ++i) {
        if (json[i] != '"' ||
            memcmp(json + i + 1, key, key_length) != 0 ||
            json[i + key_length + 1] != '"') continue;
        size_t p = i + key_length + 2;
        while (p < length && (json[p] == ' ' || json[p] == '\t' ||
                              json[p] == '\r' || json[p] == '\n')) ++p;
        if (p >= length || json[p++] != ':') continue;
        while (p < length && (json[p] == ' ' || json[p] == '\t' ||
                              json[p] == '\r' || json[p] == '\n')) ++p;
        return p < length ? json + p : NULL;
    }
    return NULL;
}

static bool json_bool(const char *json, size_t length, const char *key) {
    const char *value = find_value(json, length, key);
    return value && (value[0] == '1' ||
                     (value + 4 <= json + length &&
                      memcmp(value, "true", 4) == 0));
}

static bool json_string_is(const char *json, size_t length, const char *key,
                           const char *expected) {
    const char *value = find_value(json, length, key);
    size_t expected_length = strlen(expected);
    return value && value < json + length && value[0] == '"' &&
           value + expected_length + 2 <= json + length &&
           memcmp(value + 1, expected, expected_length) == 0 &&
           value[expected_length + 1] == '"';
}

static void set_bit(uint32_t *mask, uint8_t index, bool active) {
    if (index >= g_button_count) return;
    if (active) *mask |= 1u << index;
}

void edd_buttons_init(void) { s_active_mask = 0; }

uint32_t edd_buttons_handle_json(const char *json, size_t length) {
    if (!json ||
        (!json_string_is(json, length, "responsetype", "indicator") &&
         !json_string_is(json, length, "responsetype", "indicatorpush"))) {
        return 0;
    }

    uint32_t next = 0;
    set_bit(&next, 0, json_bool(json, length, "LandingGear"));
    set_bit(&next, 2, json_bool(json, length, "Supercruise") ||
                      json_bool(json, length, "FsdCharging") ||
                      json_bool(json, length, "FsdJump"));
    set_bit(&next, 3, json_bool(json, length, "Lights"));
    set_bit(&next, 4, json_bool(json, length, "SilentRunning"));
    set_bit(&next, 5, json_bool(json, length, "NightVision"));
    set_bit(&next, 6, json_bool(json, length, "HardpointsDeployed"));
    set_bit(&next, 10, json_bool(json, length, "HUDInAnalysisMode"));
    set_bit(&next, 11, json_string_is(json, length, "GUIFocus", "FSSMode"));
    set_bit(&next, 16, json_bool(json, length, "CargoScoopDeployed"));
    set_bit(&next, 18, json_string_is(json, length, "GUIFocus", "TargetPanel"));
    set_bit(&next, 19, json_string_is(json, length, "GUIFocus", "CommsPanel"));
    set_bit(&next, 20, json_string_is(json, length, "GUIFocus", "RolePanel"));
    set_bit(&next, 21, json_string_is(json, length, "GUIFocus", "SystemPanel"));
    set_bit(&next, 22, json_string_is(json, length, "GUIFocus", "GalaxyMap"));
    set_bit(&next, 23, json_string_is(json, length, "GUIFocus", "SystemMap"));

    uint32_t changed = s_active_mask ^ next;
    s_active_mask = next;
    return changed;
}

bool edd_button_active(uint8_t index) {
    return index < 32 && (s_active_mask & (1u << index)) != 0;
}

