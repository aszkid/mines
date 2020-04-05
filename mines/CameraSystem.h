#pragma once

#include "Context.h"
#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct camera_system_t {
    camera_system_t(context_t *ctx)
		: ctx(ctx), mouse_last_mode(0)
	{}

	void update(entity_t camera)
	{
        Camera* cam = &ctx->emgr.get_component<Camera>(camera);

        int x, y;
        SDL_GetRelativeMouseState(&x, &y);
        unsigned int captured = SDL_GetRelativeMouseMode();
        if (!captured) {
            x = 0; y = 0;
        }

        const float sensitivity = 0.1f;
        cam->yaw += x * sensitivity;
        cam->pitch += - y * sensitivity;
        glm::vec3 look = cam->look();

        const uint8_t* state = SDL_GetKeyboardState(NULL);
        const glm::vec3 cam_right = glm::cross(cam->up, look);
        float vel = 15.f / 1000.f;
        const float delta = (float)ctx->delta;

        if (state[SDL_SCANCODE_LCTRL]) {
            vel *= 2.f;
        }

        glm::mat4 tr = glm::mat4(1.f);
        if (state[SDL_SCANCODE_W]) {
            tr = glm::translate(tr, vel * delta * look);
        }
        if (state[SDL_SCANCODE_S]) {
            tr = glm::translate(tr, -vel * delta * look);
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
        if (state[SDL_SCANCODE_ESCAPE]) {
            uint32_t now = SDL_GetTicks();
            if (now - mouse_last_mode > 200) {
                SDL_SetRelativeMouseMode((SDL_bool)(!captured));
                mouse_last_mode = now;
            }
        }

        cam->pos = glm::vec3(tr * glm::vec4(cam->pos, 1.f));
	}

	context_t* ctx;
    uint32_t mouse_last_mode;
};