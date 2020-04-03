#include "InputSystem.h"
#include "Context.h"
#include "Triangle.h"
#include <SDL.h>

input_system_t::input_system_t(context_t* ctx)
	: ctx(ctx)
{
}

input_system_t::~input_system_t()
{
}

int input_system_t::init()
{
	return 0;
}

int input_system_t::update()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT:
            return 1;
        case SDL_KEYDOWN:
            if (e.key.keysym.scancode == SDL_SCANCODE_UP) {
                ctx->emgr.insert_component<Triangle>({ 0, 0 }, {
                    -0.5f, -0.5f, 0.0f,
                     0.5f, -0.5f, 0.0f,
                     0.0f,  0.5f, 0.0f
                    });
            } else if (e.key.keysym.scancode == SDL_SCANCODE_DOWN) {
                ctx->emgr.delete_component<Triangle>({ 0, 0 });
            } else if (e.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                ctx->emgr.delete_component<Triangle>({ 0, 2 });
            } else if (e.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                ctx->emgr.insert_component<Triangle>({ 0, 2 }, {
                    -0.5f,  0.0f, 0.0f,
                     0.5f,  0.0f, 0.0f,
                     0.0f,  1.0f, 0.0f
                });
            }
            break;
        default:
            break;
        }
    }

    return 0;
}
