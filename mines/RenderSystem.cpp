#include "RenderSystem.h"
#include "Context.h"
#include "Triangle.h"
#include <cstdio>

enum STATUS {
    RS_UP = 0,
    RS_DOWN,
    RS_ERR_SDLINIT,
    RS_ERR_SDLWIN,
    RS_ERR_GL,
    RS_ERR_GLAD,
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

static GLuint load_shader()
{
    GLuint vsID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fsID = glCreateShader(GL_FRAGMENT_SHADER);

    std::string vs_code = "#version 330 core\n"
        "layout(location=0) in vec3 vpos;\n"
        "void main() {\n"
        "   gl_Position.xyz = vpos;\n"
        "   gl_Position.w = 1.0;\n"
        "}\n";
    std::string fs_code = "#version 330 core\n"
        "out vec4 color;\n"
        "void main() {\n"
        "   color = vec4(0.0,0.0,1.0,1.0);\n"
        "}\n";

    char const* vs_ptr = vs_code.c_str();
    glShaderSource(vsID, 1, &vs_ptr, NULL);
    glCompileShader(vsID);

    GLint Result = GL_FALSE;
    int InfoLogLength;
    glGetShaderiv(vsID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(vsID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(vsID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        std::printf("vs error: %s\n", &VertexShaderErrorMessage[0]);
    }

    char const* fs_ptr = fs_code.c_str();
    glShaderSource(fsID, 1, &fs_ptr, NULL);
    glCompileShader(fsID);
    glGetShaderiv(fsID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(fsID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(fsID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        std::printf("fs error: %s\n", &FragmentShaderErrorMessage[0]);
    }

    GLuint pID = glCreateProgram();
    glAttachShader(pID, vsID);
    glAttachShader(pID, fsID);
    glLinkProgram(pID);

    glGetProgramiv(pID, GL_LINK_STATUS, &Result);
    glGetProgramiv(pID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(pID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        std::printf("p error: %s\n", &ProgramErrorMessage[0]);
    }

    glDetachShader(pID, vsID);
    glDetachShader(pID, fsID);
    glDeleteShader(vsID);
    glDeleteShader(fsID);

    return pID;
}

int render_system_t::init()
{
    status = RS_UP;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        std::printf("SDL_Init error: %s\n", SDL_GetError());
        status = RS_ERR_SDLINIT;
        return status;
    }

    ctx->win = SDL_CreateWindow("mines", 100, 100, ctx->width, ctx->height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (ctx->win == nullptr) {
        std::printf("SDL_CreateWindow error: %s\n", SDL_GetError());
        status = RS_ERR_SDLWIN;
        return status;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24); // ?

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

    if (!gladLoadGL()) {
        std::printf("gladLoadGL failed!\n");
        status = RS_ERR_GLAD;
        return status;
    }

    glClearColor(0.5f, 0.f, 0.f, 1.f);

    shader = load_shader();

    return status;
}

static GLuint new_triangle(Triangle *t)
{
    GLuint vao, vbo;
    // generate VBO and VAO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    // bind VAO, start describing it
    glBindVertexArray(vao);
    // has a VBO bound
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Triangle), t, GL_STATIC_DRAW);
    // enable and set first vertex attribute (position)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, //first vertex attribute
        3, // has 3 components
        GL_FLOAT, // each of type GL_FLOAT
        GL_FALSE, // should not be normalized
        3 * sizeof(float), // space between consecutive attributes
        nullptr // offset of the first attribute in the buffer
    );
    return vao;
}

void render_system_t::render()
{
    // process backlogged events
    state_stream_t* ss = ctx->emgr.get_state_stream<Triangle>();
    for (auto& msg : ss->events_back) {
        // can safely access components directly: these changes
        // have been materialized by the entity manager
        switch (msg.type) {
        case state_msg_header_t::C_NEW:
            vaos.emplace(msg.e, new_triangle(&ctx->emgr.get_component<Triangle>(msg.e)));
            break;
        default:
            break;
        }
    }

    glClear(GL_COLOR_BUFFER_BIT);
    /////////////////////////////////
    // render here
    glUseProgram(shader);
    for (auto& vao : vaos) {
        glBindVertexArray(vao.second);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    /////////////////////////////////
    SDL_GL_SwapWindow(ctx->win);
}
