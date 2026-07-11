#include "buttons.h"

const button_definition_t g_buttons[] = {
    {"LAND",    "GEAR",       0x0F, icon_landing_gear_normal, icon_landing_gear_active},
    {"SPEED",   "0%",         0x1B, icon_supercruise_decelerate_normal, icon_supercruise_decelerate_active},
    {"FSD",     "JUMP",       0x0D, icon_hyperspace_normal, icon_hyperspace_active},
    {"",        "LIGHTS",     0x49, icon_ship_lights_normal, icon_ship_lights_active},
    {"",        "SILENT",     0x4C, icon_silent_running_normal, icon_silent_running_active},
    {"NIGHT",   "VISION",     0x30, icon_night_vision_normal, icon_night_vision_active},

    {"HARD",    "PTS",        0x18, icon_hardpoints_normal, icon_hardpoints_active},
    {"FIRE",    "GROUP",      0x11, icon_next_firegroup_normal, icon_next_firegroup_active},
    {"",        "TARGET",     0x17, icon_target_normal, icon_target_active},
    {"NEXT",    "TARGET",     0x0A, icon_target_next_normal, icon_target_next_active},
    {"ANALYS",  "COMBAT",     0x10, icon_analysis_mode_normal, icon_analysis_mode_active},
    {"FSS",     "SCAN",       0x34, icon_fss_normal, icon_fss_active},

    {"SHIELD",  "POWER",      0x50, icon_shield_power_normal, icon_shield_power_active},
    {"ENGINE",  "POWER",      0x52, icon_engine_power_normal, icon_engine_power_active},
    {"WEAPON",  "POWER",      0x4F, icon_weapon_power_normal, icon_weapon_power_active},
    {"BAL",     "POWER",      0x51, icon_balance_power_normal, icon_balance_power_active},
    {"CARGO",   "SCOOP",      0x4A, icon_cargo_scoop_normal, icon_cargo_scoop_active},
    {"CARGO",   "EJECT",      0x38, icon_eject_cargo_normal, icon_eject_cargo_active},

    {"TARGET",  "PANEL",      0x1E, icon_exploration_normal, icon_exploration_active},
    {"COMMS",   "PANEL",      0x1F, icon_comms_panel_normal, icon_comms_panel_active},
    {"ROLE",    "PANEL",      0x20, icon_nav_panel_normal, icon_nav_panel_active},
    {"SYSTEM",  "PANEL",      0x21, icon_trade_normal, icon_trade_active},
    {"GALAXY",  "MAP",        0x36, icon_galaxy_map_normal, icon_galaxy_map_active},
    {"SYSTEM",  "MAP",        0x37, icon_system_map_normal, icon_system_map_active},
};

const size_t g_button_count = sizeof(g_buttons) / sizeof(g_buttons[0]);
