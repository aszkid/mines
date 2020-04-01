#pragma once

#include <glad/glad.h>
#include <vector>

struct context_t;

class render_system_t {
public:
	render_system_t(context_t* ctx);
	~render_system_t();

	int init();
	void render();

private:
	int status;
	context_t* ctx;
	std::vector<GLuint> vaos;
	GLuint shader;
};

