// Single translation unit that compiles the Vulkan Memory Allocator library.
// Kept apart from render.cpp so VMA's implementation is built exactly once and
// its warnings don't touch our own code.
#include <vulkan/vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
