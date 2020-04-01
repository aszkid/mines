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
    entity_t d = ctx.emgr.new_entity();
    entity_t e = ctx.emgr.new_entity();

    auto arr2 = packed_array_t::make<float, 10>();
    arr2.emplace(a, 3.f);
    arr2.emplace(b, 9.f);
    arr2.emplace(c, 18.f);
    arr2.emplace(d, 27.f);
    arr2.print<float>();

    std::printf("removing b...\n");
    arr2.remove(b);
    std::printf("inserting e...\n");
    arr2.emplace(e, 81.f);
    arr2.print<float>();
    assert(arr2.has(b) == false);

    auto pair = arr2.any<float>();
    for (size_t i = 0; i < arr2.size(); i++) {
        std::printf("data[%zu] (entity=%zu) = %f\n",
            i, (uint32_t)pair.first[i], pair.second[i]
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
