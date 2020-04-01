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

    std::array<entity_t, 10> ents;
    for (size_t i = 0; i < ents.size(); i++) {
        ents[i] = ctx.emgr.new_entity();
        ctx.emgr.attach_component<Position>(ents[i], { (float)i, -(float)i });
        if (i % 2 == 0) {
            ctx.emgr.attach_component<Velocity>(ents[i], { (float)5*i });
        }
    }

    auto join = ctx.emgr.join<Position, Velocity>();
    for (entity_t& e : join) {
        std::printf("entity(%zu) pos=%f, vel=%f\n",
            (uint32_t)e,
            ctx.emgr.get_component<Position>(e).x,
            ctx.emgr.get_component<Velocity>(e).vx
        );
    }

    if (render_sys.init() != 0)
        return EXIT_FAILURE;

    while (!quit) {
        if (input_sys.update() != 0)
            break;

        render_sys.render();
    }

    return EXIT_SUCCESS;
}
