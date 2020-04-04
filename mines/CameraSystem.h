#pragma once

#include "Context.h"
#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct camera_system_t {
    camera_system_t(context_t *ctx)
		: ctx(ctx)
	{}

	void update(entity_t camera)
	{
        Camera* cam = &ctx->emgr.get_component<Camera>(camera);

        const uint8_t* state = SDL_GetKeyboardState(NULL);
        const glm::vec3 cam_right = glm::cross(cam->up, cam->look);
        const float vel = 1.f / 1000.f;
        const float delta = (float)ctx->delta;

        glm::mat4 tr = glm::mat4(1.f);
        if (state[SDL_SCANCODE_W]) {
            tr = glm::translate(tr, vel * delta * cam->look);
        }
        if (state[SDL_SCANCODE_S]) {
            tr = glm::translate(tr, -vel * delta * cam->look);
        }
        if (state[SDL_SCANCODE_A]) {
            tr = glm::translate(tr, vel * delta * cam_right);
        }
        if (state[SDL_SCANCODE_D]) {
            tr = glm::translate(tr, -vel * delta * cam_right);
        }
        if (state[SDL_SCANCODE_SPACE]) {
            tr = glm::translate(tr, vel * delta * cam->up);
        }
        if (state[SDL_SCANCODE_LSHIFT]) {
            tr = glm::translate(tr, - vel * delta * cam->up);
        }

        cam->pos = glm::vec3(tr * glm::vec4(cam->pos, 1.f));
	}

	context_t* ctx;
};