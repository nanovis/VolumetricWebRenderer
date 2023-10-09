#pragma once

#include <string>
#include <map>
#include <vector>
#include <array>
#include <webgpu/webgpu_cpp.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VolumeLoader.h"
#include "VolumeHelper.h"
#include "AnnotationHandler.h"
#include "PlaneRenderer.h"
#include "SegmentationUpdateHandler.h"

#ifndef __EMSCRIPTEN__
	#include "imgui/imgui.h"
	#include "imgui/imgui_impl_glfw.h"
	#include "imgui/imgui_impl_wgpu.h"
#endif

class Camera;

#ifdef __EMSCRIPTEN__
class VolumeRenderer
#else
class VolumeRenderer
#endif
{
public: 


	/**
	 * Constructs a new Volume Renderer object.
	 * 
	 */
	VolumeRenderer();

	/**
	 * Destroys the Volume Render object, shuts down Dear ImGui, and releases WebGPU objects. 
	 * 
	 */
	virtual ~VolumeRenderer();

	/**
	 * Resizes Volume Renderer.
	 * Resizing will initialize the Volume Renderer again.
	 */
	void resize(int width, int height);

	/**
	 * Initializes the Volume Renderer.
	 * This includes the setup of shaders, the creation of resources, the setup of the volume and effect pipeline.
	 */
	void init();

	/**
	 * Render routine
	 * @param timeElapsed The elapsed time.
	 */
	void render(double timeElapsed, const wgpu::TextureView &renderTarget);

#ifndef __EMSCRIPTEN__
	/**
	 * Sets up the Dear ImGui Interface.
	 * @param window The GLFWwindow.
	 */
	void setupImGui(GLFWwindow *window);
#endif

	void reload();

	/**
	 * Sets the wgpu device.
	 * @param swapchain The wgpu device.
	 */
	inline void setDevice(wgpu::Device device) { this->device = device; }
	
	/**
	 * Sets the preferred swap chain texture format.
	 * @param format The preferred texture format.
	 */
	inline void setPreferredSwapChainTextureFormat(wgpu::TextureFormat format) { this->preferredSwapChainTextureFormat = format; }
	
	/**
	 * Sets the Volume Loader.
	 * @param volumeLoader Pointer to volumeLoader.
	 */
	//inline void setVolumeLoader(VolumeLoader *volumeLoader) { this->volumeLoader = volumeLoader; }

	/**
	 * Sets the camera.
	 * @param camera Pointer to camera.
	 */
	inline void setCamera(Camera *camera) { this->camera = camera; }

	/**
	 * Sets the width.
	 * @param width Width in pixels.
	 */
	inline void setWidth(int width) { this->width = width; }
	
	/**
	 * Sets the height.
	 * @param height Height in pixels
	 */
	inline void setHeight(int height) { this->height = height; }

	/**
	 * Returns true if annotations are enabled.
	 */
	inline bool isAnnotationEnabled() { return enableAnnotations; }

	/**
	 * Specifies if ambient occlusion is enabled.
	 */
	inline void enableAO(bool enable) { enableAmbientOcclusion = enable; }

	/**
	 * Uploads the volume descriptors to the GPU.
	 * @param volumeDescriptors Vector of volume descriptors.
	 */
	void uploadVolumeDescriptors(const std::vector<VolumeDescriptor>& volumeDescriptors);

	/**
	 * Opens volumme data.
	 * @param file The config file.
	 * @param openBySlice Specifies if the data is streamed to the GPU by slices, or uploaded at once.
	 */
	void openVolume(std::string &file, bool openBySlice = true);

	/**
	 * Updates a segmentation mask of the current volume data.
	 * @param which Specifies the index of the mask that is updated. 
	 * @param maskFileName path of the file.
	 */
	void updateSegmentationMask(int which, std::string maskFileName);
	
	/**
	 * Enables/disables clipping and chooses the clipping plane axis.
	 * @param modus The modus: 0: clipping disabled, 1: view-aligned, 2: x-axis, 3: y-axis, 4: z-axis 
	 */
	void setClippingPlaneModus(int modus);

	/**
	 * Saves the current parameters of the session into a file.
	 * @param file_name The path of the file
	 */
	void saveSession(std::string &file_name);

	/**
	 * Loads the parameters from a file into the current session.
	 * @param file_name The path of the file
	 */
	void loadSession(std::string file);

	/**
	 * Changes a transfer function according to the parameters.
	 * @param which Specifies which transfer function is changed.
	 * @param lowRamp The low value of the ramp function.
	 * @param highRamp The high value of the ramp function.
	 * @param color The color of the transfer function.
	 */
	void setTransferFunction(int which, float lowRamp, float highRamp, glm::vec3 color);

	/**
	 * Sets the background color.
	 * @param r Red [0..1]
	 * @param g Green [0..1]
	 * @param b Blue [0..1]
	 */
	inline void setClearColor(float r, float g, float b) { clear_color = glm::vec4(r, g, b, 1.0f); }

	inline void setClippingPlaneRenderer (PlaneRenderer *renderer) { clippingPlaneRenderer = renderer; }

	inline void setAnnotationHandler(AnnotationHandler* handler) { this->annotationHandler = handler; }

	inline void setClippingMaskA(bool enabled) { clipMaskA = enabled; }

	inline void setClippingMaskB(bool enabled) { clipMaskB = enabled; }
	
	inline void setClippingMaskC(bool enabled) { clipMaskC = enabled; }

	inline void setAnnotationKernelSize(int size) { annotationKernelSize = size; }

	inline void setClippingPlaneOrigin(glm::vec4 origin) { clippingPlaneOrigin = origin; }

	inline void setClippingPlaneNormal(glm::vec4 normal) { clippingPlaneNormal = normal; }

	inline void setInteractionMode(bool enabled) { interactionMode = enabled; }

	inline void toggleFullscreen() { enableFullscreenAnnotations = !enableFullscreenAnnotations; }

	inline void toggleVolumeA() { enableVolumeA = !enableVolumeA; }

	inline void toggleVolumeB() { enableVolumeB = !enableVolumeB; }

	inline void toggleVolumeC() { enableVolumeC = !enableVolumeC; }
	
	inline std::vector<VolumeDescriptor> getVolumeDescriptors() { return volumeDescriptors; }

	glm::mat4 modelMatrix = glm::mat3(1.0);
	
private:

#ifndef __EMSCRIPTEN__
	void composeImGui();
	void renderImGUI(const wgpu::TextureView& renderTarget);
#endif

	void preRender(double timeElapsed);

	void renderVolumes(double timeElapsed, const wgpu::TextureView& renderTarget, wgpu::CommandEncoder *encoder);

	void renderPostprocessing(double timeElapsed, const wgpu::TextureView& renderTarget, wgpu::CommandEncoder* encoder);

	wgpu::RenderPipeline createRenderPipeline(std::string label, wgpu::ShaderModule vs, wgpu::ShaderModule fs, wgpu::PipelineLayout pipelineLayout,
		wgpu::VertexBufferLayout vertexBufferLayout, wgpu::TextureFormat textureFormat, bool ennableDepthStencil = false);

	void setupShaders();

	void createVolumeResources();

	wgpu::TextureView createTextureView(int source, bool rgba);

	void createEffectResources();

	void createAnnotationResources();

	wgpu::RenderPipeline setupVolumePipelineLayout();

	wgpu::RenderPipeline setupEffectPipelineLayout(wgpu::ShaderModule vsModule, wgpu::ShaderModule fsModule, wgpu::TextureView inputTextureView, wgpu::TextureView inputTexture2View, wgpu::Buffer paramBuffer);

	std::pair<double, double> mousePos;
	
	AnnotationHandler *annotationHandler = nullptr;
	PlaneRenderer* clippingPlaneRenderer = nullptr;
	SegmentationUpdateHandler *segmentationUpdateHandler = nullptr;
	Camera *camera = nullptr;

	// volume transfer functions
	std::vector<glm::vec4> transferFunctionColor;
	std::vector<float> transferFunctionRampLow;
	std::vector<float> transferFunctionRampHigh;

	// texture format of the render target
	wgpu::TextureFormat preferredSwapChainTextureFormat;
	
	// shaders
	std::array<wgpu::ShaderModule, 2> volumeShaderModules;
	std::vector<std::array<wgpu::ShaderModule, 2>> effectShaderModules;

	// background color
	glm::vec4 clear_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	// webgpu device
	wgpu::Device device;

	// textures
	std::vector<wgpu::Texture> volumeTextures;
	wgpu::Texture depthStencilTexture;
	wgpu::Texture annotationTexturePing;
	wgpu::Texture annotationTexturePong;

	// texture views
	std::vector<wgpu::TextureView> volumeTextureViews;
	wgpu::TextureView depthStencilTextureView;
	wgpu::TextureView annotationTexturePingView;
	wgpu::TextureView annotationTexturePongView;
	std::vector<wgpu::TextureView> effectTextureViews;

	// sampler
	wgpu::Sampler volumeTextureSampler;
	wgpu::Sampler effectTextureSampler;

	// pipelines
	wgpu::RenderPipeline volumePipeline;
	std::vector<wgpu::RenderPipeline> effectPipelines;
	
	// GPU buffers
	wgpu::Buffer vertexBuffer;
	wgpu::Buffer indexBuffer;
	wgpu::Buffer cameraBuffer;
	wgpu::Buffer paramBuffer;
	wgpu::Buffer transformBuffer[2];
	wgpu::Buffer volumeRatiosBuffer;
	wgpu::Buffer transferFunctionBufferRamp1;
	wgpu::Buffer transferFunctionBufferRamp2;
	wgpu::Buffer transferFunctionBufferColor;
	std::vector<wgpu::Buffer> effectParamBuffers;
	
	// bind groups
	wgpu::BindGroup volumeBindGroup;
	std::vector<wgpu::BindGroup> effectsBindGroup;

	// volume meta data
	std::vector<VolumeDescriptor> volumeDescriptors;
	std::vector<glm::vec3> volumeRatios;
	
	// clipping plane
	glm::vec4 clippingPlaneOrigin;
	glm::vec4 clippingPlaneNormal;
	
	// width and height of the render texture
	int width;
	int height;
	bool initialized = false;
	bool enabled = true;
	bool enableAnnotations = false;
	bool enableFullscreenAnnotations = false;
	bool enableEarlyRayTermination = true;
	bool enableJittering = true;
	bool enableAmbientOcclusion = true;
	bool enableSoftShadows = true;
	bool enablePostProcessing = true;
	bool clippingViewAligned = false;
	bool showImGui = true;
	bool enableClipping = false;
	bool clipMaskA = true;
	bool clipMaskB = true;
	bool clipMaskC = true;
	bool enableVolumeA = true;
	bool enableVolumeB = true;
	bool enableVolumeC = true;
	bool enableVolumeD = true;
	bool interactionMode = false;

	float clippingPlaneOffset = 0.0f;
	float paddingRatioX = 0.0;
	float sampleRate = 5.0f;
	float aoRadius = 1.0f;
	float aoStrength = 0.9f;
	float voxelSize = 1.0f;
	float nearPlane = 0.01f;
	float farPlane = 100.0f;
	float shadowQuality = 1.0f;
	float shadowStrength = 0.5f;
	float shadowRadius = 0.2f;

	int annotationKernelSize = 25;
	int numVolumes = 0;
	int whichMask = 0;
	int aoNumSamples = 5;
	int annotationVolume = 0;
	int clippingAxis = 0;

	// NOTE: depth attachment can be disabled to avoid current emscripten bug, however, it removes some clipping features
	bool enableDepthAttachment = true;
	bool enableStencil = false;
};

