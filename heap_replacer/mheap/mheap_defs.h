#pragma once

#define POOL_ALIGN (0x01000000u)
#define POOL_BLOCK_SIZE (0x00400000u)

#define POOL_MAX_ALLOC_SIZE (3584u)

#define POOL_SIZE_ARRAY_LEN (((POOL_MAX_ALLOC_SIZE) >> 2u) + 1u)
#define POOL_ADDR_ARRAY_LEN ((0x80000000u / (POOL_ALIGN)) << 1u)
#define POOL_INDX_ARRAY_LEN (countof(pool_desc))
