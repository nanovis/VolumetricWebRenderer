#pragma once

#include <webgpu/webgpu_cpp.h>


namespace utils 
{
    wgpu::Buffer CreateBuffer(const wgpu::Device& device, uint64_t size, wgpu::BufferUsage usage);
} // namespace utils

