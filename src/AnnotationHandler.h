#pragma once

#include "Camera.h"

#include <vector>
#include <webgpu/webgpu_cpp.h>
#include "VolumeHelper.h"

// define is used so that json throws exceptions
#include <json/json.hpp>
using json = nlohmann::json;

struct Stroke
{
	bool add = true;
	std::string volume_name;
	glm::vec3 kernelSize;
	std::vector<glm::vec3> positions;
};

class AnnotationHandler
{
public:

	AnnotationHandler(wgpu::Device device, Camera* camera, int width, int height);

	void init();

	void resize(int width, int height);

	void reload();
	
	void processAnnotations();

	inline void setVolumeRatio(glm::vec3 volumeRatio) { this->volumeRatio = volumeRatio; }

	inline void setVolumeSize(glm::uvec4 volumeSize) { this->volumeSize = volumeSize; }

	void setTexturePing(wgpu::Texture texture);
	
	void setTexturePong(wgpu::Texture texture);

	void serialize(std::string file_name);

	inline bool isPingPong() { return pingPong; }

	inline void setCamera(Camera *camera) { this->camera = camera; }

	inline void setKernelSize(glm::vec3 size) {
		newStroke = size != kernelSize || newStroke; kernelSize = size;
	}

	inline void setAddAnnotation(bool add) {
		newStroke = add != addAnnotation || newStroke; addAnnotation = add;
	}

	glm::vec4 clippingPlaneOrigin;
	glm::vec4 clippingPlaneNormal;
	std::vector<glm::vec3> volumeRatios;
	bool newStroke = true;

	glm::mat4 volumeModelMatrix = glm::mat4(1.0f);

	int annotationVolume = 0;

	std::vector<VolumeDescriptor> volumeDescriptors;

private:

	float prev_pos_x = -1;
	float prev_pos_y = -1;

	bool addAnnotation = true;

	wgpu::ComputePipeline setupAnnotationPipelineLayout(bool pingPong);

	void applyAnnotations(glm::vec3 vertex, bool copy);

	json annotations;

	Camera *camera = nullptr;
	bool pingPong = true;
	int width;
	int height;

	bool initialized = false;

	glm::vec3 kernelSize;
	glm::vec3 volumeRatio;
	glm::uvec4 volumeSize;

	std::vector<Stroke> strokes;
	
	wgpu::Device device;
	wgpu::ShaderModule annotationShaderModule;
	wgpu::BindGroup annotationBindGroup[2];
	wgpu::Buffer annotationParamBuffer;
	wgpu::Texture annotationTexturePing;
	wgpu::Texture annotationTexturePong;
	wgpu::TextureView annotationTexturePingView;
	wgpu::TextureView annotationTexturePongView;
	wgpu::ComputePipeline annotationPipelinePing;
	wgpu::ComputePipeline annotationPipelinePong;
};

