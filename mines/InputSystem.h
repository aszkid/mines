#pragma once

#include <cstdint>

struct context_t;

class input_system_t {
public:
	input_system_t(context_t* ctx)
		: ctx(ctx)
	{}

	int update()
	{
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				return 1;
			case SDL_KEYDOWN:
				if (e.key.keysym.scancode == SDL_SCANCODE_Q)
					return 1;
			default:
				break;
			}
		}

		return 0;
	}
	
private:
	context_t* ctx;
};

