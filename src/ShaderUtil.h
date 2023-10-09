#pragma once

#include <webgpu/webgpu_cpp.h>

#include <string>

class ShaderUtil
{
public:

	static std::array<wgpu::ShaderModule, 2> loadAndCompileShader(wgpu::Device &device, std::string& vsFile, std::string& fsFile);
	
	static wgpu::ShaderModule loadAndCompileShader(wgpu::Device &device, std::string& computeFile);

private:

	static std::string loadShader(std::string file, bool& ok);

};

