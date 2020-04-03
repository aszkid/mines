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

int main(int argc, char **argv)
{
    bool quit = false;

    context_t ctx;
    ctx.width = 640;
    ctx.height = 480;

    render_system_t render_sys(&ctx);
    input_system_t input_sys(&ctx);
    asset_manager_t assets;

    const asset_t cube_mesh("/cube.obj"_hash);
    const asset_t blank_material("/blank.mat"_hash);
    // preload assets
    assets.load(cube_mesh, nullptr);
    assets.load(blank_material, nullptr);

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

    entity_t player;
    ctx.emgr.new_entity(&player, 1);
    ctx.emgr.insert_component<RenderMesh>(player, {
        cube_mesh, blank_material
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
