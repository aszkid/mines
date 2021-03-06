#pragma once

#include <SDL.h>

#include "AssetManager.h"
#include "EntityManager.h"
#include "Config.h"

struct context_t {
    SDL_Window* win;
    int width, height;
    SDL_GLContext gl_ctx;

    entity_manager_t emgr;
    asset_manager_t assets;

    // time last frame was in flight
    uint32_t delta;
    uint32_t mouse_last_mode;

    config_t cfg;
};