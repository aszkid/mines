#include "InputSystem.h"
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
        default:
            break;
        }
    }

    return 0;
}
