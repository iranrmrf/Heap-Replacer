#pragma once

#include "main/util.h"

constexpr size_t default_heap_cell_size = 4u * KB;
constexpr size_t default_heap_block_size = 128u * MB;
constexpr size_t default_heap_max_size = 2u * GB;

constexpr size_t default_heap_block_count = default_heap_max_size / default_heap_block_size;
constexpr size_t default_heap_cell_count = default_heap_block_size / default_heap_cell_size;

constexpr size_t index_bits = bit_index(default_heap_block_count);
constexpr size_t size_bits = 32u - index_bits;
