#include <cstdio>
#include <cstdlib>
#include <memory>

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

    entity_t a = ctx.emgr.new_entity();
    entity_t b = ctx.emgr.new_entity();
    entity_t c = ctx.emgr.new_entity();

    const uint32_t VEL_ID = "VELOCITY"_hash;
    ctx.emgr.attach_component<float>(a, VEL_ID, 3.f);
    ctx.emgr.attach_component<float>(b, VEL_ID, 9.f);
    ctx.emgr.attach_component<float>(c, VEL_ID, 18.f);

    const uint32_t POS_ID = "POSITION"_hash;
    ctx.emgr.attach_component<float>(a, POS_ID, 99.f);
    ctx.emgr.attach_component<float>(c, POS_ID, -5.f);

    auto join = ctx.emgr.join(POS_ID, VEL_ID);
    for (entity_t& e : join) {
        std::printf("entity(%zu) pos=%f, vel=%f\n",
            (uint32_t)e,
            ctx.emgr.get_component<float>(e, POS_ID),
            ctx.emgr.get_component<float>(e, VEL_ID)
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
