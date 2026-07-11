#include "edd_state.h"

#include <stdlib.h>
#include <string.h>

#include "jsmn.h"

#define TOKEN_COUNT 512

/* Keep the parser workspace out of the Pico's small main stack. */
static jsmntok_t s_tokens[TOKEN_COUNT];

static bool token_equals(const char *json, const jsmntok_t *token,
                         const char *text) {
    size_t length = (size_t)(token->end - token->start);
    return token->type == JSMN_STRING && strlen(text) == length &&
           memcmp(json + token->start, text, length) == 0;
}

static int skip_token(const jsmntok_t *tokens, int count, int index) {
    int end = tokens[index].end;
    ++index;
    while (index < count && tokens[index].start < end) ++index;
    return index;
}

static int array_item(const jsmntok_t *tokens, int count, int array,
                      int ordinal) {
    if (array < 0 || array >= count || tokens[array].type != JSMN_ARRAY)
        return -1;
    int item = array + 1;
    for (int i = 0; i < ordinal; ++i) {
        if (item >= count || tokens[item].start >= tokens[array].end) return -1;
        item = skip_token(tokens, count, item);
    }
    return item < count && tokens[item].start < tokens[array].end ? item : -1;
}

static int find_key(const char *json, const jsmntok_t *tokens, int count,
                    int object, const char *key) {
    if (object < 0 || object >= count || tokens[object].type != JSMN_OBJECT)
        return -1;
    int i = object + 1;
    while (i + 1 < count && tokens[i].start < tokens[object].end) {
        int value = i + 1;
        if (token_equals(json, &tokens[i], key)) return value;
        i = skip_token(tokens, count, value);
    }
    return -1;
}

static void copy_token(const char *json, const jsmntok_t *token, char *dest,
                       size_t capacity) {
    if (!dest || !capacity || !token || token->start < 0) return;
    size_t out = 0;
    for (int i = token->start; i < token->end && out + 1 < capacity; ++i) {
        char c = json[i];
        if (c == '\\' && i + 1 < token->end) {
            char escaped = json[++i];
            if (escaped == 'n' || escaped == 'r' || escaped == 't') c = ' ';
            else if (escaped == 'u' && i + 4 < token->end) {
                i += 4;
                c = '?';
            } else c = escaped;
        }
        dest[out++] = c;
    }
    dest[out] = 0;
}

static void copy_key(const char *json, const jsmntok_t *tokens, int count,
                     int object, const char *key, char *dest, size_t capacity) {
    int value = find_key(json, tokens, count, object, key);
    if (value >= 0) copy_token(json, &tokens[value], dest, capacity);
}

static void copy_string_key(const char *json, const jsmntok_t *tokens,
                            int count, int object, const char *key,
                            char *dest, size_t capacity) {
    int value = find_key(json, tokens, count, object, key);
    if (value >= 0 && tokens[value].type == JSMN_STRING) {
        copy_token(json, &tokens[value], dest, capacity);
    }
}

static int integer_key(const char *json, const jsmntok_t *tokens, int count,
                       int object, const char *key, int fallback) {
    int value = find_key(json, tokens, count, object, key);
    if (value < 0) return fallback;
    char number[20];
    copy_token(json, &tokens[value], number, sizeof(number));
    return atoi(number);
}

static bool bool_key(const char *json, const jsmntok_t *tokens, int count,
                     int object, const char *key, bool fallback) {
    int value = find_key(json, tokens, count, object, key);
    if (value < 0) return fallback;
    int length = tokens[value].end - tokens[value].start;
    const char *data = json + tokens[value].start;
    return (length == 4 && memcmp(data, "true", 4) == 0) ||
           (length == 1 && data[0] == '1');
}

void edd_state_init(edd_state_t *state) {
    memset(state, 0, sizeof(*state));
    state->remaining_jumps = -1;
}

static void parse_status(edd_state_t *state, const char *json,
                         const jsmntok_t *t, int count) {
    int system = find_key(json, t, count, 0, "SystemData");
    int eddb = find_key(json, t, count, 0, "EDDB");
    int ship = find_key(json, t, count, 0, "Ship");
    int travel = find_key(json, t, count, 0, "Travel");
    copy_key(json, t, count, system, "System", state->system, sizeof(state->system));
    copy_key(json, t, count, eddb, "Allegiance", state->allegiance, sizeof(state->allegiance));
    copy_key(json, t, count, eddb, "Economy", state->economy, sizeof(state->economy));
    copy_key(json, t, count, eddb, "Security", state->security, sizeof(state->security));
    copy_key(json, t, count, ship, "Ship", state->ship, sizeof(state->ship));
    copy_key(json, t, count, ship, "Fuel", state->fuel, sizeof(state->fuel));
    copy_key(json, t, count, ship, "TankSize", state->tank_size, sizeof(state->tank_size));
    copy_key(json, t, count, ship, "Range", state->jump_range, sizeof(state->jump_range));
    copy_key(json, t, count, ship, "Cargo", state->cargo, sizeof(state->cargo));
    copy_key(json, t, count, travel, "Dist", state->travel_dist, sizeof(state->travel_dist));
    copy_key(json, t, count, travel, "Jumps", state->travel_jumps, sizeof(state->travel_jumps));
    copy_key(json, t, count, travel, "Time", state->travel_time, sizeof(state->travel_time));
    copy_key(json, t, count, 0, "Bodyname", state->body, sizeof(state->body));
    copy_key(json, t, count, 0, "StationName", state->station, sizeof(state->station));
    copy_key(json, t, count, 0, "Commander", state->commander, sizeof(state->commander));
    copy_key(json, t, count, 0, "Mode", state->mode, sizeof(state->mode));
    copy_key(json, t, count, 0, "GameMode", state->game_mode, sizeof(state->game_mode));
    copy_key(json, t, count, 0, "HomeDist", state->home_dist, sizeof(state->home_dist));
    copy_key(json, t, count, 0, "SolDist", state->sol_dist, sizeof(state->sol_dist));
    copy_key(json, t, count, 0, "Credits", state->credits, sizeof(state->credits));
    /*
     * EDDiscovery reports JSON null outside the short jump transition.  That
     * is not a route clear, so it must not erase the persistent next system
     * obtained from FSDTarget or journal recovery.
     */
    copy_string_key(json, t, count, 0, "FSDJumpNextSystemName",
                    state->next_system, sizeof(state->next_system));
    state->remaining_jumps = integer_key(json, t, count, 0, "RemainingJumps",
                                         state->remaining_jumps);
}

static void parse_indicator(edd_state_t *state, const char *json,
                            const jsmntok_t *t, int count) {
    state->mass_locked = bool_key(json, t, count, 0, "FsdMassLocked", state->mass_locked);
    state->shields_up = bool_key(json, t, count, 0, "ShieldsUp", state->shields_up);
    state->low_fuel = bool_key(json, t, count, 0, "LowFuel", state->low_fuel);
    state->overheat = bool_key(json, t, count, 0, "OverHeating", state->overheat);
    state->danger = bool_key(json, t, count, 0, "IsInDanger", state->danger);
    state->interdicted = bool_key(json, t, count, 0, "BeingInterdicted", state->interdicted);
    copy_key(json, t, count, 0, "LegalState", state->legal_state, sizeof(state->legal_state));
    int pips = find_key(json, t, count, 0, "Pips");
    bool valid_pips = bool_key(json, t, count, 0, "ValidPips", true);
    if (!valid_pips) {
        state->pips[0] = state->pips[1] = state->pips[2] = 0;
    } else if (pips >= 0 && t[pips].type == JSMN_ARRAY) {
        int item = pips + 1;
        for (int i = 0; i < 3 && item < count && t[item].start < t[pips].end; ++i) {
            char value[12];
            copy_token(json, &t[item], value, sizeof(value));
            state->pips[i] = atoi(value);
            item = skip_token(t, count, item);
        }
    }
}

static void parse_generic_ui(edd_state_t *state, const char *json,
                             const jsmntok_t *t, int count) {
    int data = find_key(json, t, count, 0, "data");
    int event_type = find_key(json, t, count, data, "EventTypeID");
    char event_time[sizeof(state->route_event_time)] = {0};
    copy_key(json, t, count, data, "EventTimeUTC", event_time,
             sizeof(event_time));
    if (event_type >= 0 && token_equals(json, &t[event_type], "NavRouteClear")) {
        state->remaining_jumps = -1;
        state->route_target[0] = 0;
        state->next_system[0] = 0;
        if (event_time[0] &&
            (!state->route_event_time[0] ||
             strcmp(event_time, state->route_event_time) > 0)) {
            strcpy(state->route_event_time, event_time);
        }
        return;
    }
    int target = find_key(json, t, count, data, "FSDTarget");
    if (target >= 0) {
        state->remaining_jumps = integer_key(json, t, count, target,
            "RemainingJumpsInRoute", state->remaining_jumps);
        if (state->remaining_jumps == 0) {
            state->next_system[0] = 0;
        } else {
            copy_string_key(json, t, count, target, "StarSystem",
                            state->next_system, sizeof(state->next_system));
        }
        if (event_time[0] &&
            (!state->route_event_time[0] ||
             strcmp(event_time, state->route_event_time) > 0)) {
            strcpy(state->route_event_time, event_time);
        }
    }
}

static void copy_array_text(const char *json, const jsmntok_t *t, int count,
                            int row, int column, char *destination,
                            size_t capacity) {
    int value = array_item(t, count, row, column);
    if (value >= 0) copy_token(json, &t[value], destination, capacity);
}

static bool apply_journal_clear(edd_state_t *state, const char *event_time) {
    if (state->route_event_time[0] && event_time[0] &&
        strcmp(event_time, state->route_event_time) < 0) {
        return false;
    }
    state->remaining_jumps = -1;
    state->route_target[0] = 0;
    state->next_system[0] = 0;
    if (event_time[0]) strcpy(state->route_event_time, event_time);
    return true;
}

static bool apply_journal_route(edd_state_t *state, int remaining,
                                const char *destination,
                                const char *next_system,
                                const char *event_time) {
    bool newer_route = !state->route_event_time[0] ||
                       (event_time[0] &&
                        strcmp(event_time, state->route_event_time) > 0);

    /*
     * History can still contain the original NavRoute count after FSDTarget
     * supplied a newer live value. The same/older route may only decrease the
     * count. A genuinely newer NavRoute is a replot and may increase it.
     */
    if (!newer_route &&
        (state->remaining_jumps < 0 || remaining > state->remaining_jumps)) {
        return false;
    }

    state->remaining_jumps = remaining;
    if (destination[0]) strcpy(state->route_target, destination);
    if (remaining == 0) {
        state->next_system[0] = 0;
    } else if (next_system[0]) {
        strcpy(state->next_system, next_system);
    }
    if (newer_route && event_time[0]) strcpy(state->route_event_time, event_time);
    return true;
}

static void copy_route_segment(const char *json, const char *start,
                               const char *end, char *destination,
                               size_t capacity) {
    while (start < end && (*start == ' ' || *start == '\t')) ++start;
    while (end > start && (end[-1] == ' ' || end[-1] == '\t')) --end;
    jsmntok_t token = {
        .type = JSMN_STRING,
        .start = (int)(start - json),
        .end = (int)(end - json),
        .size = 0,
    };
    copy_token(json, &token, destination, capacity);
}

static bool parse_journal_route(edd_state_t *state, const char *json,
                                const jsmntok_t *t, int count) {
    int rows = find_key(json, t, count, 0, "rows");
    if (rows < 0 || t[rows].type != JSMN_ARRAY) return false;

    int completed_jumps = 0;
    int row = rows + 1;
    while (row < count && t[row].start < t[rows].end) {
        if (t[row].type == JSMN_ARRAY) {
            int event = array_item(t, count, row, 0);
            if (event >= 0 && token_equals(json, &t[event], "Journal.FSDJump")) {
                ++completed_jumps;
                row = skip_token(t, count, row);
                continue;
            }
            if (event >= 0 &&
                (token_equals(json, &t[event], "Journal.NavRouteClear") ||
                 token_equals(json, &t[event], "Journal.FSDTargetClear"))) {
                char event_time[sizeof(state->route_event_time)] = {0};
                copy_array_text(json, t, count, row, 1, event_time,
                                sizeof(event_time));
                return apply_journal_clear(state, event_time);
            }
            if (event >= 0 && token_equals(json, &t[event], "Journal.NavRoute")) {
                int summary = array_item(t, count, row, 3);
                if (summary < 0 || t[summary].type != JSMN_STRING) return false;

                const char *start = json + t[summary].start;
                const char *end = json + t[summary].end;
                char *number_end = NULL;
                long jumps = strtol(start, &number_end, 10);
                if (number_end == start || jumps < 0 || jumps > 100000) return false;
                int remaining = (int)jumps - completed_jumps;
                if (remaining < 0) remaining = 0;

                /* EDDiscovery formats this as "N jumps: A, B, destination". */
                const char *route_start = memchr(start, ':', (size_t)(end - start));
                if (!route_start) return false;
                ++route_start;

                const char *destination_start = route_start;
                for (const char *p = route_start; p < end; ++p) {
                    if (*p == ',') destination_start = p + 1;
                }
                char destination[sizeof(state->route_target)] = {0};
                copy_route_segment(json, destination_start, end, destination,
                                   sizeof(destination));

                /*
                 * Rows are newest-first. Each FSDJump seen before this
                 * NavRoute advances one item in the saved route list.
                 */
                const char *next_start = route_start;
                for (int i = 0; i < completed_jumps && next_start < end; ++i) {
                    const char *comma = memchr(next_start, ',',
                                               (size_t)(end - next_start));
                    next_start = comma ? comma + 1 : end;
                }
                const char *next_end = next_start < end
                    ? memchr(next_start, ',', (size_t)(end - next_start))
                    : NULL;
                if (!next_end) next_end = end;
                char next_name[sizeof(state->next_system)] = {0};
                if (remaining > 0 && next_start < end) {
                    copy_route_segment(json, next_start, next_end, next_name,
                                       sizeof(next_name));
                    /* Long EDDiscovery summaries may abbreviate the middle. */
                    if (strcmp(next_name, "..") == 0) next_name[0] = 0;
                }

                char event_time[sizeof(state->route_event_time)] = {0};
                copy_array_text(json, t, count, row, 1, event_time,
                                sizeof(event_time));
                return apply_journal_route(state, remaining, destination,
                                           next_name, event_time);
            }
        }
        row = skip_token(t, count, row);
    }
    return false;
}

bool edd_state_handle_json(edd_state_t *state, const char *json, size_t length) {
    jsmn_parser parser;
    jsmn_init(&parser);
    int count = jsmn_parse(&parser, json, length, s_tokens, TOKEN_COUNT);
    ++state->messages_received;
    if (count < 1 || s_tokens[0].type != JSMN_OBJECT) return false;
    int response = find_key(json, s_tokens, count, 0, "responsetype");
    if (response < 0) return false;
    bool recognized = false;
    if (token_equals(json, &s_tokens[response], "status")) {
        parse_status(state, json, s_tokens, count);
        recognized = true;
    } else if (token_equals(json, &s_tokens[response], "indicator") ||
               token_equals(json, &s_tokens[response], "indicatorpush")) {
        parse_indicator(state, json, s_tokens, count);
        recognized = true;
    } else if (token_equals(json, &s_tokens[response], "genericui")) {
        parse_generic_ui(state, json, s_tokens, count);
        recognized = true;
    } else if (token_equals(json, &s_tokens[response], "journalrequest") ||
               token_equals(json, &s_tokens[response], "journalpush") ||
               token_equals(json, &s_tokens[response], "journalrefresh")) {
        recognized = parse_journal_route(state, json, s_tokens, count);
    }
    if (recognized) ++state->generation;
    return recognized;
}
