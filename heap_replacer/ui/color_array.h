#pragma once

#include <time.h>

#include "main/util.h"

struct col4
{

	union
	{
		uint hex;
		struct
		{
			uchar r;
			uchar g;
			uchar b;
			uchar a;
		};
	};

	col4() { col4(0u); }
	col4(uint hex) { this->hex = hex; }
	col4(uchar r, uchar g, uchar b) { col4(r, g, b, 255u); }
	col4(uchar r, uchar g, uchar b, uchar a) { this->r = r; this->g = g; this->b = b; this->a = a; }

};

class color_array
{

private:

	col4* colors;
	size_t size;

public:

	color_array(size_t size);
	~color_array();

	void randomize();

	col4& operator[](size_t index);

};
