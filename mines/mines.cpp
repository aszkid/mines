#include <cstdio>
#include <cstdlib>
#include <memory>
#include <array>

#include "Context.h"
#include "RenderSystem.h"
#include "InputSystem.h"
#include "CameraSystem.h"
#include "MapSystem.h"

#include "Position.h"
#include "Mesh.h"
#include "RenderMesh.h"
#include "Camera.h"

int main(int argc, char **argv)
{
    bool quit = false;

    context_t ctx;
    ctx.width = 1280;
    ctx.height = 720;

    render_system_t render_sys(&ctx);
    input_system_t input_sys(&ctx);
    camera_system_t camera_sys(&ctx);
    map_system_t map_sys(&ctx);

    entity_t camera;
    ctx.emgr.new_entity(&camera, 1);
    ctx.emgr.insert_component<Camera>(camera, {
        glm::vec3(3.f, 1.5f, 3.f),   // position
        glm::vec3(0.f, 1.f, 0.f),   // up
        0.f, -90.f, 0.f,            // pitch, yaw, roll
        45.f                        // fov
        });

    if (render_sys.init() != 0)
        return EXIT_FAILURE;

    // quirk: must materialize components before map initializes
    //  (because it needs the camera)
    ctx.emgr.materialize();
    map_sys.init(camera);

    const asset_t monkey("/mesh/monkey"_hash);
    if (load_mesh(&ctx.assets, monkey, "./monkey.obj") != 0)
        return EXIT_FAILURE;

    const size_t w = 20;
    const size_t n_foos = w * w;
    entity_t foos[n_foos];
    ctx.emgr.new_entity(foos, n_foos);
    for (size_t i = 0; i < n_foos; i++) {
        ctx.emgr.insert_component<RenderMesh>(foos[i], { monkey });
        ctx.emgr.insert_component<Position>(foos[i], {
            glm::vec3(3.5f * (float)(i / w), 0.f, - 3.f * (float)(i % w))
        });
    }

    ctx.assets.print();

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
        map_sys.update(camera);
        if (input_sys.update() != 0)
            break;
        render_sys.render(camera);
    }

    render_sys.teardown();

    return EXIT_SUCCESS;
}
