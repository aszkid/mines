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
#include "RenderMesh.h"
#include "IndexedRenderMesh.h"
#include "Camera.h"
#include "Position.h"

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
    : ctx(ctx), status(RS_DOWN), cmds(sizeof(cmd_t), 0x4000), indexed_cmds(sizeof(indexed_cmd_t), 0x4000), shader(0)
{
}

static void cleanup(render_system_t *sys)
{
    glDeleteProgram(sys->shader);

    render_system_t::cmd_t* cmds = sys->cmds.any<render_system_t::cmd_t>();
    for (size_t i = 0; i < sys->cmds.size(); i++) {
        auto& cmd = cmds[i];
        glDeleteBuffers(1, &cmd.vbo);
        glDeleteVertexArrays(1, &cmd.vao);
    }

    render_system_t::indexed_cmd_t* idx_cmds = sys->indexed_cmds.any<render_system_t::indexed_cmd_t>();
    for (size_t i = 0; i < sys->indexed_cmds.size(); i++) {
        auto& cmd = idx_cmds[i];
        glDeleteBuffers(1, &cmd.vbo);
        glDeleteBuffers(1, &cmd.ebo);
        glDeleteVertexArrays(1, &cmd.vao);
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

static void handle_new_rendermesh(render_system_t* sys, entity_t e)
{
    RenderMesh* rm = &sys->ctx->emgr.get_component<RenderMesh>(e);
    Mesh* mesh = sys->ctx->assets.get<Mesh>(rm->mesh);

    render_system_t::cmd_t cmd;
    glGenVertexArrays(1, &cmd.vao);
    glGenBuffers(1, &cmd.vbo);

    glBindVertexArray(cmd.vao);
    glBindBuffer(GL_ARRAY_BUFFER, cmd.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Mesh::Vertex) * mesh->num_verts, mesh->vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, Mesh::Vertex::nx));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, Mesh::Vertex::r));

    cmd.num_verts = mesh->num_verts;
    sys->cmds.emplace(e, cmd);
}

static void handle_new_indexedrendermesh(render_system_t* sys, entity_t e)
{
    IndexedRenderMesh* rm = &sys->ctx->emgr.get_component<IndexedRenderMesh>(e);
    IndexedMesh* mesh = sys->ctx->assets.get<IndexedMesh>(rm->indexed_mesh);

    render_system_t::indexed_cmd_t cmd;
    glGenVertexArrays(1, &cmd.vao);
    glGenBuffers(1, &cmd.vbo);
    glGenBuffers(1, &cmd.ebo);

    glBindVertexArray(cmd.vao);
    glBindBuffer(GL_ARRAY_BUFFER, cmd.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(IndexedMesh::Vertex) * mesh->num_verts, mesh->vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cmd.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh->num_indices, mesh->indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(IndexedMesh::Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(IndexedMesh::Vertex), (void*)offsetof(IndexedMesh::Vertex, IndexedMesh::Vertex::nx));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(IndexedMesh::Vertex), (void*)offsetof(IndexedMesh::Vertex, IndexedMesh::Vertex::r));

    cmd.num_triangles = mesh->num_indices;
    cmd.last_update = rm->last_update;
    sys->indexed_cmds.emplace(e, cmd);
}

static void handle_update_rendermesh(render_system_t* sys, entity_t e)
{
    // TODO
}

static void handle_delete_rendermesh(render_system_t* sys, entity_t e)
{
    // TODO
}

static void handle_update_indexedrendermesh(render_system_t* sys, entity_t e)
{
    IndexedRenderMesh* rm = &sys->ctx->emgr.get_component<IndexedRenderMesh>(e);
    IndexedMesh* mesh = sys->ctx->assets.get<IndexedMesh>(rm->indexed_mesh);
    render_system_t::indexed_cmd_t& cmd = sys->indexed_cmds.get<render_system_t::indexed_cmd_t>(e);

    if (cmd.last_update >= rm->last_update)
        return;

    // grow buffer
    if (mesh->num_indices > cmd.num_triangles) {
        glBindBuffer(GL_ARRAY_BUFFER, cmd.vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(IndexedMesh::Vertex) * mesh->num_verts, mesh->vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cmd.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * mesh->num_indices, mesh->indices, GL_STATIC_DRAW);

    } else {
        glBindBuffer(GL_ARRAY_BUFFER, cmd.vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(IndexedMesh::Vertex) * mesh->num_verts, mesh->vertices);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cmd.ebo);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(unsigned int) * mesh->num_indices, mesh->indices);
    }

    cmd.num_triangles = mesh->num_indices;
    cmd.last_update = rm->last_update;
}

void render_system_t::render(entity_t camera)
{
    state_stream_t* ss;
    size_t tris = 0;
    ZoneScoped;

    {
        ZoneScoped("ss_process");
        // process RenderMesh changes
        ss = ctx->emgr.get_state_stream<RenderMesh>();
        for (auto& msg : ss->events_back) {
            switch (msg.type) {
            case state_msg_header_t::C_NEW:
                handle_new_rendermesh(this, msg.e);
                break;
            case state_msg_header_t::C_UPDATE:
                handle_update_rendermesh(this, msg.e);
                break;
            case state_msg_header_t::C_DELETE:
                handle_delete_rendermesh(this, msg.e);
                break;
            }
        }

        // process IndexedRenderMesh changes
        ss = ctx->emgr.get_state_stream<IndexedRenderMesh>();
        for (auto& msg : ss->events_back) {
            switch (msg.type) {
            case state_msg_header_t::C_NEW:
                handle_new_indexedrendermesh(this, msg.e);
                break;
            case state_msg_header_t::C_UPDATE:
                handle_update_indexedrendermesh(this, msg.e);
                break;
            default:
                // TODO update and delete
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

    {
        ZoneScoped("draw1");

        // draw commands
        std::pair<entity_t*, cmd_t*> cmd_arr = cmds.any_pair<cmd_t>();
        for (size_t i = 0; i < cmds.size(); i++) {
            auto& cmd = cmd_arr.second[i];
            entity_t e = cmd_arr.first[i];
            glm::mat4 model = glm::mat4(1.f);
            if (ctx->emgr.has_component<Position>(e)) {
                Position& pos = ctx->emgr.get_component<Position>(e);
                model = glm::translate(model, pos.pos);
            }
            RenderMesh& rm = ctx->emgr.get_component<RenderMesh>(e);
            glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(cmd.vao);
            glDrawArrays(GL_TRIANGLES, 0, cmd.num_verts);
            tris += cmd.num_verts / 3;
        }
    }

    std::vector<std::tuple<entity_t, indexed_cmd_t, glm::mat4, float, glm::vec3>> render_data;
    {
        ZoneScoped("pre_draw2");

        // insert render data
        std::pair<entity_t*, indexed_cmd_t*> idx_cmd_arr = indexed_cmds.any_pair<indexed_cmd_t>();
        for (size_t i = 0; i < indexed_cmds.size(); i++) {
            auto& cmd = idx_cmd_arr.second[i];
            entity_t e = idx_cmd_arr.first[i];
            glm::mat4 model = glm::mat4(1.f);
            auto& irm = ctx->emgr.get_component<IndexedRenderMesh>(e);
            if (!irm.visible)
                continue;
            if (cmd.num_triangles == 0)
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
            model = glm::translate(model, botL);
            render_data.emplace_back(e, cmd, model, dist, botL);
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
            glDrawElements(GL_TRIANGLES, cmd.num_triangles, GL_UNSIGNED_INT, (void*)0);
            tris += cmd.num_triangles / 3;
        }
    }

    {
        ZoneScoped("swap");
        SDL_GL_SwapWindow(ctx->win);
    }

    FrameMark;
    /////////////////////////////////
}
