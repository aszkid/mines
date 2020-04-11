#include "RenderSystem.h"

// general
#include "Context.h"
#include <cstdio>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include "utils.h"
#include <Tracy.hpp>

// components
#include "IndexedRenderMesh.h"
#include "Camera.h"
#include "Position.h"
#include "RenderModel.h"

// assets
#include "Mesh.h"
#include "IndexedMesh.h"

enum STATUS {
    RS_UP = 0,
    RS_DOWN,
    RS_ERR_SDLINIT,
    RS_ERR_SDLWIN,
    RS_ERR_GL,
    RS_ERR_GLAD,
};

render_system_t::render_system_t(context_t* ctx)
    : ctx(ctx), status(RS_DOWN), shader(0)
{
}

static void cleanup(render_system_t *sys)
{
    glDeleteProgram(sys->shader);

    for (auto& pair : sys->cmds) {
        for (auto& cmd : pair.second) {
            glDeleteBuffers(1, &cmd.vbo);
            glDeleteBuffers(1, &cmd.ebo);
            glDeleteVertexArrays(1, &cmd.vao);
        }
    }
}

void render_system_t::teardown()
{
    switch (status) {
    case RS_UP:
        // clean everything up!
        cleanup(this);
        SDL_GL_DeleteContext(ctx->gl_ctx);
    case RS_ERR_GLAD:
    case RS_ERR_GL:
        // failed to create OpenGL context or load function pointers
        SDL_DestroyWindow(ctx->win);
    case RS_ERR_SDLWIN:
        // failed to create a window
        SDL_Quit();
    case RS_ERR_SDLINIT:
    case RS_DOWN:
        // failed to even initialize SDL, or never started!
        break;
    }
}

std::string read_file(const char* fname)
{
    std::ifstream f(fname);
    f.seekg(0, std::ios::end);
    size_t sz = f.tellg();
    std::string buffer(sz, ' ');
    f.seekg(0);
    f.read(&buffer[0], sz);
    return buffer;
}

static GLuint load_shader(const char *vs, const char *fs)
{
    GLuint vsID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fsID = glCreateShader(GL_FRAGMENT_SHADER);

    std::string vs_code(read_file(vs));
    std::string fs_code(read_file(fs));

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

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

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

    if (!gladLoadGL()) {
        std::printf("gladLoadGL failed!\n");
        status = RS_ERR_GLAD;
        return status;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.8f, 0.9f, 0.9f, 1.f);

    shader = load_shader("./phong.vert", "./phong.frag");

    return status;
}

static void handle_update_indexedrendermesh(render_system_t* sys, render_system_t::cmd_t &cmd, uint32_t last_update)
{
    if (cmd.last_update >= last_update)
        return;

    glBindBuffer(GL_ARRAY_BUFFER, cmd.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cmd.ebo);

    // grow buffer if need be
    if (cmd.mesh->num_indices > cmd.num_indices) {
        glBufferData(GL_ARRAY_BUFFER, sizeof(IndexedMesh::Vertex) * cmd.mesh->num_verts, cmd.mesh->vertices, GL_STATIC_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * cmd.mesh->num_indices, cmd.mesh->indices, GL_STATIC_DRAW);

    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(IndexedMesh::Vertex) * cmd.mesh->num_verts, cmd.mesh->vertices);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(unsigned int) * cmd.mesh->num_indices, cmd.mesh->indices);
    }

    cmd.num_indices = cmd.mesh->num_indices;
    cmd.last_update = last_update;
}

static void handle_new_indexedrendermesh(render_system_t* sys, render_system_t::cmd_t& cmd, IndexedMesh* mesh, uint32_t last_update)
{
    // create buffers
    glGenVertexArrays(1, &cmd.vao);
    glGenBuffers(1, &cmd.vbo);
    glGenBuffers(1, &cmd.ebo);

    // store command
    cmd.last_update = 0;
    cmd.num_indices = 0;
    cmd.mesh = mesh;

    // upload data
    glBindVertexArray(cmd.vao);
    handle_update_indexedrendermesh(sys, cmd, last_update);

    // describe vertex format
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(IndexedMesh::Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(IndexedMesh::Vertex), (void*)offsetof(IndexedMesh::Vertex, IndexedMesh::Vertex::nx));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(IndexedMesh::Vertex), (void*)offsetof(IndexedMesh::Vertex, IndexedMesh::Vertex::r));

    // unbind vao -- important! (i believe)
    glBindVertexArray(0);
}

void render_system_t::render(entity_t camera)
{
    ZoneScoped;

    {
        ZoneScoped("ss_process");
        state_stream_t* ss;

        ss = ctx->emgr.get_state_stream<IndexedRenderMesh>();
        for (auto& msg : ss->events_back) {
            IndexedRenderMesh* irm = nullptr;
            cmd_t* cmd = nullptr;
            switch (msg.type) {
            case state_msg_header_t::C_NEW:
                irm = &ctx->emgr.get_component<IndexedRenderMesh>(msg.e);
                if (cmds.find(msg.e) == cmds.end())
                    cmds.insert({ msg.e, {} });
                cmds[msg.e].push_back({});
                handle_new_indexedrendermesh(this, cmds[msg.e].back(), ctx->assets.get<IndexedMesh>(irm->indexed_mesh), irm->last_update);
                break;
            case state_msg_header_t::C_UPDATE:
                irm = &ctx->emgr.get_component<IndexedRenderMesh>(msg.e);
                for (auto& cmd : cmds[msg.e])
                    handle_update_indexedrendermesh(this, cmd, irm->last_update);
                break;
            default:
                // TODO delete
                break;
            }
        }
        
        ss = ctx->emgr.get_state_stream<RenderModel>();
        for (auto& msg : ss->events_back) {
            RenderModel* rm = nullptr;
            switch (msg.type) {
            case state_msg_header_t::C_NEW:
                rm = &ctx->emgr.get_component<RenderModel>(msg.e);
                if (cmds.find(msg.e) == cmds.end())
                    cmds.insert({ msg.e, {} });
                for (unsigned i = 0; i < rm->num_meshes; i++) {
                    cmds[msg.e].push_back({});
                    handle_new_indexedrendermesh(this, cmds[msg.e].back(), ctx->assets.get<IndexedMesh>(rm->meshes[i]), rm->last_update);
                }
                break;
            case state_msg_header_t::C_UPDATE:
                rm = &ctx->emgr.get_component<RenderModel>(msg.e);
                assert(cmds.find(msg.e) != cmds.end());
                for (auto& cmd : cmds[msg.e])
                    handle_update_indexedrendermesh(this, cmd, rm->last_update);
                break;
            default:
                // TODO delete
                break;
            }
        }
    }

    int projection_loc, model_loc, view_loc, lightpos_loc, lightcol_loc, viewpos_loc;
    glm::mat4 view, projection;
    Camera* cam;

    {
        ZoneScoped("render_setup");

        cam = &ctx->emgr.get_component<Camera>(camera);
        view = glm::lookAt(cam->pos, cam->pos + cam->look(), cam->up);
        projection = glm::perspectiveFov(cam->fov, (float)ctx->width, (float)ctx->height, 0.1f, 1000.f);

        /////////////////////////////////
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);

        // get uniform locations
        projection_loc = glGetUniformLocation(shader, "projection");
        model_loc = glGetUniformLocation(shader, "model");
        view_loc = glGetUniformLocation(shader, "view");
        lightpos_loc = glGetUniformLocation(shader, "lightPos");
        lightcol_loc = glGetUniformLocation(shader, "lightColor");
        viewpos_loc = glGetUniformLocation(shader, "viewPos");

        // set uniform values
        glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3f(lightpos_loc, -10.f, 64.f, 32.f);
        glUniform3f(lightcol_loc, 1.f, 1.f, 1.f);
        glUniform3f(viewpos_loc, cam->pos.x, cam->pos.y, cam->pos.z);
    }

    std::vector<std::tuple<entity_t, cmd_t, glm::mat4, float, glm::vec3>> render_data;
    {
        ZoneScoped("pre_draw2");

        // insert render data
        for (auto& pair : cmds) {
            entity_t e = pair.first;
            auto& rm = ctx->emgr.get_component<RenderModel>(e);
            if (!rm.visible || rm.num_meshes == 0)
                continue;
            ////////////////////////////////////////////////////////////////
            // TODO: actually do frustrum culling right,
            //       this is waaayy too ad-hoc
            glm::vec3 botL = ctx->emgr.get_component<Position>(e).pos;
            glm::vec3 topR = botL + glm::vec3(32.f, 32.f, 32.f);
            float dist = glm::distance(cam->pos, botL);
            if (glm::dot(cam->pos - botL, cam->look()) > 0 && glm::dot(cam->pos - topR, cam->look()) > 0)
                continue;
            ////////////////////////////////////////////////////////////////
            glm::mat4 model = glm::translate(glm::mat4(1.f), botL);
            for (auto& cmd : pair.second) {
                if (cmd.num_indices == 0)
                    continue;
                render_data.emplace_back(e, cmd, model, dist, botL);
            }
        }

        // sort render data by distance
        static const auto sort_func = [](const auto& a, const auto& b) -> bool {
            return std::get<3>(a) < std::get<3>(b);
        };
        std::sort(render_data.begin(), render_data.end(), sort_func);
    }
    {
        ZoneScoped("draw2");

        // draw render data
        for (auto& tup : render_data) {
            auto& cmd = std::get<1>(tup);
            glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(std::get<2>(tup)));
            glBindVertexArray(cmd.vao);
            glDrawElements(GL_TRIANGLES, cmd.num_indices, GL_UNSIGNED_INT, (void*)0);
            glBindVertexArray(0);
        }
    }

    {
        ZoneScoped("swap");
        SDL_GL_SwapWindow(ctx->win);
    }

    FrameMark;
    /////////////////////////////////
}
