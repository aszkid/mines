#include <cstdio>
#include <cstdlib>
#include <memory>
#include <array>

#include "Context.h"
#include "EntityManager.h"
#include "PackedArray.h"

#include "RenderSystem.h"
#include "InputSystem.h"

#include "Triangle.h"
#include "RenderMesh.h"

#include "MeshLoader.cpp"

int main(int argc, char **argv)
{
    bool quit = false;

    context_t ctx;
    ctx.width = 640;
    ctx.height = 480;

    render_system_t render_sys(&ctx);
    input_system_t input_sys(&ctx);

    // preload assets
    const asset_t monkey("/mesh/monkey"_hash);
    if (load_mesh(&ctx.assets, monkey, "./monkey.obj") != 0)
        return EXIT_FAILURE;

    entity_t tris[4];
    ctx.emgr.new_entity(tris, 4);

    ctx.emgr.insert_component<Triangle>(tris[0], {
     -1.0f,  0.0f, 0.0f,
        0.0f,  0.0f, 0.0f,
        -0.5f,  1.0f, 0.0f,
    });
    ctx.emgr.insert_component<Triangle>(tris[1], {
       0.0f,  -1.0f, 0.0f,
       1.0f,  -1.0f, 0.0f,
       0.5f,  0.0f, 0.0f,
    });
    ctx.emgr.insert_component<Triangle>(tris[2], {
       0.0f,  0.0f, 0.0f,
       1.0f,  0.0f, 0.0f,
       0.5f,  1.0f, 0.0f,
    });
    ctx.emgr.insert_component<Triangle>(tris[3], {
       -1.0f,  -1.0f, 0.0f,
        0.0f,  -1.0f, 0.0f,
        -0.5f,  0.0f, 0.0f,
    });

    entity_t foo;
    ctx.emgr.new_entity(&foo, 1);
    ctx.emgr.insert_component<RenderMesh>(foo, {
        monkey, 0
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
