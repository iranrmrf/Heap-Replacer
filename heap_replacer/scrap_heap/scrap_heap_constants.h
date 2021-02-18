#pragma once

constexpr size_t scrap_heap_manager_buffer_count = 64u;

constexpr size_t scrap_heap_buffer_max_size = 0x00400000u;
constexpr size_t scrap_heap_buffer_min_size = 0x00040000u;

constexpr size_t scrap_heap_free_flag = 0x80000000u;
