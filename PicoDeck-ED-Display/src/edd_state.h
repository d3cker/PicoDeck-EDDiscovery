#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EDD_TEXT 80

typedef struct {
    char system[EDD_TEXT];
    char body[EDD_TEXT];
    char station[EDD_TEXT];
    char ship[EDD_TEXT];
    char commander[EDD_TEXT];
    char mode[EDD_TEXT];
    char game_mode[EDD_TEXT];
    char route_target[EDD_TEXT];
    char route_event_time[32];
    char next_system[EDD_TEXT];
    char fuel[EDD_TEXT];
    char tank_size[EDD_TEXT];
    char jump_range[EDD_TEXT];
    char cargo[EDD_TEXT];
    char travel_dist[EDD_TEXT];
    char travel_jumps[EDD_TEXT];
    char travel_time[EDD_TEXT];
    char home_dist[EDD_TEXT];
    char sol_dist[EDD_TEXT];
    char credits[EDD_TEXT];
    char allegiance[EDD_TEXT];
    char economy[EDD_TEXT];
    char security[EDD_TEXT];
    char legal_state[EDD_TEXT];
    int remaining_jumps;
    int pips[3];
    bool shields_up;
    bool mass_locked;
    bool low_fuel;
    bool overheat;
    bool danger;
    bool interdicted;
    uint32_t generation;
    uint32_t messages_received;
} edd_state_t;

void edd_state_init(edd_state_t *state);
bool edd_state_handle_json(edd_state_t *state, const char *json, size_t length);
