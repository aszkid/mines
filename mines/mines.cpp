#include <cstdio>
#include <cstdlib>
#include <memory>
#include <array>

#include "Context.h"
#include "EntityManager.h"
#include "PackedStore.h"
#include "PackedArray.h"

#include "RenderSystem.h"
#include "InputSystem.h"

#include "Position.h"
#include "Velocity.h"
#include "Triangle.h"

class PhysicsSystem {
public:
    PhysicsSystem()
        : data(sizeof(phys_t), 1024)
    {

    }
private:
    struct phys_t {
        Position pos;
        Velocity vel;
    };
    packed_array_t data;
};

class GraphicsSystem {
private:
    packed_array_t data;
};

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
       -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        0.0f,  1.0f, 0.0f,
    });

    if (render_sys.init() != 0)
        return EXIT_FAILURE;

    while (!quit) {
        if (input_sys.update() != 0)
            break;

        render_sys.render();
    }

    return EXIT_SUCCESS;
}
