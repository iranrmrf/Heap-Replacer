#include "color_array.h"

#ifdef HR_USE_GUI

color_array::color_array(size_t size) : colors(nullptr), size(size)
{
	this->colors = (col4*)hr::hr_calloc(size, sizeof(col4));
	this->randomize();
}

color_array::~color_array()
{
	hr::hr_free(this->colors);
}

void color_array::randomize()
{
	for (size_t i = 0u; i < this->size; i++)
	{
		size_t r = rand() % 256u;
		size_t g = rand() % 256u;
		size_t b = rand() % 256u;
		size_t a = 255u;
		this->colors[i] = (r << 24u) + (g << 16u) + (b << 8u) + a;
	}
}

col4& color_array::operator[](size_t index)
{
	return this->colors[index];
}

#endif
