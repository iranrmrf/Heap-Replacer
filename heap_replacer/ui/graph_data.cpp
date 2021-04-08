#include "graph_data.h"

#ifdef HR_USE_GUI

graph_data::graph_data(size_t count) : values(nullptr), offset(0u), count(count), alloc(0u),
	running_min(0.0f), running_max(0.0f), window_min(0.0f), window_max(0.0f)
{
	this->values = (float*)hr::hr_calloc(this->count, sizeof(float));
	this->alloc = this->count;
}

graph_data::~graph_data()
{
	hr::hr_free(this->values);
}

void graph_data::add_data(float data)
{
	if (this->alloc != this->count)
	{
		this->values = (float*)hr::hr_realloc(this->values, this->count * sizeof(float));
		this->alloc = this->count;
		this->offset %= this->count;
	}
	if (data < this->running_min) { this->running_min = data; }
	if (data > this->running_max) { this->running_max = data; }
	if (this->window_min == this->values[this->offset])
	{
		this->window_min = data;
		for (size_t i = 0u; i < this->count; i++)
		{
			if (this->values[i] < this->window_min) { this->window_min = this->values[i]; }
		}
	}
	if (this->window_max == this->values[this->offset])
	{
		this->window_max = data;
		for (size_t i = 0u; i < this->count; i++)
		{
			if (this->values[i] > this->window_max) { this->window_min = this->values[i]; }
		}
	}
	this->values[this->offset++] = data;
	this->offset %= this->count;
}

float graph_data::get_back()
{
	if (this->offset) { return this->values[this->offset - 1u]; }
	return this->values[this->count - 1u];
}

#endif
