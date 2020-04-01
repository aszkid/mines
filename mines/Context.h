#pragma once

#include <SDL.h>
#include <SDL_opengl.h>
#include <gl/GLU.h>

#include "EntityManager.h"
#include "InputSystem.h"
#include "RenderSystem.h"

struct context_t {
    SDL_Window* win;
    int width, height;
    SDL_GLContext gl_ctx;

    entity_manager_t emgr;
};