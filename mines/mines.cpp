#include <cstdio>
#include <cstdlib>
#include <memory>
#include <array>

#include "Context.h"
#include "EntityManager.h"
#include "PackedArray.h"

#include "RenderSystem.h"
#include "InputSystem.h"

#include "Position.h"
#include "Velocity.h"
#include "Triangle.h"

int main(int argc, char **argv)
{
    bool quit = false;

    context_t ctx;
    ctx.width = 640;
    ctx.height = 480;

    render_system_t render_sys(&ctx);
    input_system_t input_sys(&ctx);

    entity_t tri = ctx.emgr.new_entity();
    ctx.emgr.attach_component<Triangle>(tri, {
     -1.0f,  0.0f, 0.0f,
        0.0f,  0.0f, 0.0f,
        -0.5f,  1.0f, 0.0f,
    });
    entity_t tri2 = ctx.emgr.new_entity();
    ctx.emgr.attach_component<Triangle>(tri2, {
       0.0f,  -1.0f, 0.0f,
       1.0f,  -1.0f, 0.0f,
       0.5f,  0.0f, 0.0f,
    });

    if (render_sys.init() != 0)
        return EXIT_FAILURE;

    while (!quit) {
        if (input_sys.update() != 0)
            break;

        render_sys.render();
        ctx.emgr.materialize();
    }

    return EXIT_SUCCESS;
}