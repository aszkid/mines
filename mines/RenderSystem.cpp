#include "RenderSystem.h"
#include "Context.h"
#include <cstdio>

enum STATUS {
    RS_UP = 0,
    RS_DOWN,
    RS_ERR_SDLINIT,
    RS_ERR_SDLWIN,
    RS_ERR_GL,
};

render_system_t::render_system_t(context_t* ctx)
    : ctx(ctx), status(RS_DOWN)
{
}

render_system_t::~render_system_t()
{
    switch (status) {
    case RS_ERR_SDLINIT:
        break;
    case RS_ERR_SDLWIN:
        SDL_Quit();
    case RS_ERR_GL:
        SDL_DestroyWindow(ctx->win);
    }
}

int render_system_t::init()
{
    status = RS_UP;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        std::printf("SDL_Init error: %s\n", SDL_GetError());
        status = RS_ERR_SDLINIT;
        return status;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    ctx->win = SDL_CreateWindow("mines", 100, 100, ctx->width, ctx->height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (ctx->win == nullptr) {
        std::printf("SDL_CreateWindow error: %s\n", SDL_GetError());
        status = RS_ERR_SDLWIN;
        return status;
    }

    ctx->gl_ctx = SDL_GL_CreateContext(ctx->win);
    if (ctx->gl_ctx == nullptr) {
        std::printf("SDL_GL_CreateContext error: %s\n", SDL_GetError());
        status = RS_ERR_GL;
        return status;
    }

    if (SDL_GL_SetSwapInterval(1) != 0) {
        std::printf("SDL_GL_SetSwapInterval error: %s\n", SDL_GetError());
        status = RS_ERR_GL;
        return status;
    }

    glDisable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.f, 0.f, 1.f);
    glViewport(0, 0, ctx->width, ctx->height);

    return status;
}

void render_system_t::render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(ctx->win);
}
