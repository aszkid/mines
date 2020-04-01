#pragma once

struct context_t;

class input_system_t {
public:
	input_system_t(context_t* ctx);
	~input_system_t();

	int init();
	int update();
	
private:
	context_t* ctx;
};

