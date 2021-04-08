#pragma once

#include "main/util.h"

struct graph_data
{

public:

	float* values;

	size_t offset;
	size_t count;
	size_t alloc;

	float running_min;
	float running_max;

	float window_min;
	float window_max;

public:

	graph_data(size_t count);
	~graph_data();

	void add_data(float data);
	float get_back();

};
