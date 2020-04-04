#include <cstdio>
#include <cstdlib>
#include <memory>
#include <array>

#include "Context.h"
#include "RenderSystem.h"
#include "InputSystem.h"
#include "CameraSystem.h"

#include "Triangle.h"
#include "RenderMesh.h"
#include "Camera.h"

#include "MeshLoader.cpp"

int main(int argc, char **argv)
{
    bool quit = false;

    context_t ctx;
    ctx.width = 640;
    ctx.height = 480;

    render_system_t render_sys(&ctx);
    input_system_t input_sys(&ctx);
    camera_system_t camera_sys(&ctx);

    const asset_t monkey("/mesh/monkey"_hash);
    if (load_mesh(&ctx.assets, monkey, "./monkey.obj") != 0)
        return EXIT_FAILURE;

    entity_t foo;
    ctx.emgr.new_entity(&foo, 1);
    ctx.emgr.insert_component<RenderMesh>(foo, {
        monkey, 0
    });

    entity_t camera;
    ctx.emgr.new_entity(&camera, 1);
    ctx.emgr.insert_component<Camera>(camera, {
        glm::vec3(0.f, 0.f, 3.f),   // position
        glm::vec3(0.f, 1.f, 0.f),   // up
        0.f, -80.f, 0.f,            // pitch, yaw, roll
        45.f                        // fov
    });

    if (render_sys.init() != 0)
        return EXIT_FAILURE;

    uint32_t prev = SDL_GetTicks();
    uint32_t delta;
    while (!quit) {
        // update frame time
        uint32_t now = SDL_GetTicks();
        ctx.delta = now - prev;
        prev = now;

        // materialize previous frame
        ctx.emgr.materialize();

        // run systems
        camera_sys.update(camera);
        if (input_sys.update() != 0)
            break;
        render_sys.render(camera);
    }

    return EXIT_SUCCESS;
}
