#include "Helper.h"

#include <iostream>

namespace utils
{

wgpu::Buffer CreateBuffer(const wgpu::Device& device,
    uint64_t size,
    wgpu::BufferUsage usage) {
    wgpu::BufferDescriptor descriptor;
    descriptor.size = size;
    descriptor.usage = usage | wgpu::BufferUsage::CopyDst;

    std::cout << "allocate gpu memory: " << size << " bytes." << std::endl;
    wgpu::Buffer buffer = device.CreateBuffer(&descriptor);

    return buffer;
}
}  // namespace utils