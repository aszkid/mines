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
            break;
        default:
            break;
        }
    }

    return 0;
}
