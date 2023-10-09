#include "RendererWindow.h"
#include "VolumeRenderer.h"
#include "Input.h"
#include "AnnotationHandler.h"
#include "Hotkeys.h"

#include "CameraController.h"
#include "PlaneRenderer.h"

#include <fstream>
#include <stdio.h>
#include <chrono>
#include <list>
#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/geometric.hpp>

#include <webgpu/webgpu_cpp.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;
#endif

PlaneRenderer *clippingPlaneRenderer;
VolumeRenderer *volumeRenderer;
RendererWindow *window;
CameraController *cameraController;
Camera *camera;
WGPUDevice wgpuDevice;
wgpu::SwapChain swap_chain;
AnnotationHandler *annotationHandler;

#ifndef __EMSCRIPTEN__
	#include "FileWatchHelper.h"
	FileWatchHelper *fileWatchHelper;
#endif

bool start = true;
std::chrono::time_point<std::chrono::system_clock> prevTime;

double prev_xPos = -1;
double prev_yPos = -1;

// =================== "Binding Window to Renderer" =====================
/**
  * Callback render function passed to the window.
  */
bool render(double time)
{	
	if (start) 
	{
		prevTime = std::chrono::system_clock::now();
		start = false;
		return true;
	}
	auto currentTime = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = currentTime - prevTime;
	prevTime = currentTime;

#ifndef __EMSCRIPTEN__
	fileWatchHelper->fileWatcher.update();
	// reset error flag if volume renderer has reloaded files
	if (fileWatchHelper->reloaded)
	{
		window->errorState = 0;
		fileWatchHelper->reloaded = false;
	}
#endif

	// only render and update if no error is detected
	if (window->errorState == 0) 
	{
		auto mousePos = Input::mousePosition();
		if (prev_xPos == mousePos.first && prev_xPos == mousePos.first)
		{
			volumeRenderer->setInteractionMode(false);
		}
		prev_xPos = mousePos.first;
		prev_yPos = mousePos.second;
		

		cameraController->update(elapsed_seconds.count());
		volumeRenderer->modelMatrix = cameraController->model;
		volumeRenderer->setDevice(wgpuDevice);
		volumeRenderer->render(elapsed_seconds.count(), swap_chain.GetCurrentTextureView());
		
		clippingPlaneRenderer->volumeModelMatrix = cameraController->model;
		clippingPlaneRenderer->render(swap_chain.GetCurrentTextureView());

#ifndef __EMSCRIPTEN__
		// presents rendered image to the user
		swap_chain.Present();
#endif
	}

	return true;
}

/**
  * Callback resize function passed to the window.
  */
void resizeHandler(int width, int height)
{
	// create WGPU rendering related items
	swap_chain = wgpu::SwapChain(window->createSwapChain(wgpuDevice, width, height));
	camera->setAspectRatio(width / float(height));

	volumeRenderer->resize(width, height);	
	annotationHandler->resize(width, height);
	clippingPlaneRenderer->resize(width, height);
}

/**
  * Callback mouse click handle function passed to the window.
  */
void mouseClickHandler(int button, int action, int mods)
{
#ifndef __EMSCRIPTEN__
	ImGuiIO& io = ImGui::GetIO();
	if (!io.WantCaptureMouse)
	{
		if (action == GLFW_PRESS) 
		{
			Input::mousePressEvent(button);
			if (button == 1) {
				
			}
		}
		if (action == GLFW_RELEASE) {
			Input::mouseReleaseEvent(button);
		}
	}
#else
	if (action == GLFW_PRESS) {
		Input::mousePressEvent(button);
}
	if (action == GLFW_RELEASE) {
		Input::mouseReleaseEvent(button);
	}
#endif
}

/**
  * Callback mouse move handle function passed to the window.
  */
void mouseMovedHandler(double xPos, double yPos)
{
#ifndef __EMSCRIPTEN__
	ImGuiIO &io = ImGui::GetIO();
	if (!io.WantCaptureMouse) 
	{
#endif
		auto mousePos = Input::mousePosition();
		Input::setMousePos(xPos, yPos);
		if ((Input::getMouseButton(0) || Input::getMouseButton(1) ))
		{
			volumeRenderer->setInteractionMode(true);
			if (volumeRenderer->isAnnotationEnabled() && Input::getKey(Hotkeys::getHotkey("annotation_mode")))
			{
				annotationHandler->setAddAnnotation(Input::getMouseButton(0));
				annotationHandler->processAnnotations();
			}
		}
#ifndef __EMSCRIPTEN__
	}
#endif
}

/**
  * Callback scroll handle function passed to the window.
  */
void scrollHandler(double xOffset, double yOffset)
{
#ifndef __EMSCRIPTEN__
	ImGuiIO& io = ImGui::GetIO();
	if (!io.WantCaptureMouse)
	{
#endif
		Input::wheelEvent(xOffset, yOffset);
#ifndef __EMSCRIPTEN__
	}
#endif
}

/**
  * Callback key press handle function passed to the window.
  */
void keyPressHandler(int keyCode, int action)
{
#ifndef __EMSCRIPTEN__
	ImGuiIO& io = ImGui::GetIO();
	if (!io.WantCaptureKeyboard)
	{
#endif
		if (action == GLFW_PRESS) 
		{
			Input::keyPressEvent(keyCode);
			if (keyCode == Hotkeys::getHotkey("fullscreen"))
			{
				volumeRenderer->toggleFullscreen();
			}

			if (keyCode == Hotkeys::getHotkey("clipping-disabled"))
			{
				volumeRenderer->setClippingPlaneModus(0);
			}
			if (keyCode == Hotkeys::getHotkey("clipping-x-axis"))
			{
				volumeRenderer->setClippingPlaneModus(2);
			}
			if (keyCode == Hotkeys::getHotkey("clipping-y-axis"))
			{
				volumeRenderer->setClippingPlaneModus(3);
			}
			if (keyCode == Hotkeys::getHotkey("clipping-z-axis"))
			{
				volumeRenderer->setClippingPlaneModus(4);
			}
			if (keyCode == Hotkeys::getHotkey("clipping-view-aligned"))
			{
				volumeRenderer->setClippingPlaneModus(1);
			}
			if (keyCode == Hotkeys::getHotkey("clipping_raw_data"))
			{
				clippingPlaneRenderer->enableRawData = !clippingPlaneRenderer->enableRawData;
			}
		}

		if (action == GLFW_RELEASE) {
			Input::keyReleaseEvent(keyCode);
		}
#ifndef __EMSCRIPTEN__
	}
#endif
}
// =================== "Binding Window to Renderer" END =====================
bool started = false;
void startApp()
{
	if (started) {
		return;
	}
	std::cout << "============== Application Started =================" << std::endl;
	started = true;

	// add hotkeys
	Hotkeys::setHotkey("fullscreen", GLFW_KEY_F);
	Hotkeys::setHotkey("annotation_mode", GLFW_KEY_LEFT_CONTROL);

	Hotkeys::setHotkey("rotation_mouse", GLFW_MOUSE_BUTTON_1);
	Hotkeys::setHotkey("zoom_mouse", GLFW_MOUSE_BUTTON_MIDDLE);
	Hotkeys::setHotkey("slices_mouse", GLFW_MOUSE_BUTTON_2);

	Hotkeys::setHotkey("clipping-disabled", GLFW_KEY_0);
	Hotkeys::setHotkey("clipping-x-axis", GLFW_KEY_1);
	Hotkeys::setHotkey("clipping-y-axis", GLFW_KEY_2);
	Hotkeys::setHotkey("clipping-z-axis", GLFW_KEY_3);
	Hotkeys::setHotkey("clipping-view-aligned", GLFW_KEY_4);
	Hotkeys::setHotkey("clipping_raw_data", GLFW_KEY_SPACE);
	
	window = new RendererWindow();

	int width = 1920;
	int height = 1080;

	auto wHnd = window->create(width, height);

	if (wHnd)
	{
		auto win = window->getGLFWWindow();
		wgpuDevice = window->createDevice(wHnd);
		// create cpp device
		wgpu::Device device = wgpu::Device(wgpuDevice);

		// create WGPU rendering related items
		swap_chain = wgpu::SwapChain(window->createSwapChain(wgpuDevice, width, height));

		// bind the user interaction
		window->setCursorPosCallback(mouseMovedHandler);
		window->setMouseButtonCallback(mouseClickHandler);
		window->setKeyCallback(keyPressHandler);
		window->setScrollCallback(scrollHandler);
		window->resized(resizeHandler);

		camera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		camera->setAspectRatio(width / float(height));
		camera->setFov(45.0f);
		cameraController = new CameraController(camera);
#ifdef __EMSCRIPTEN__
		cameraController->scrollSpeed = -0.05f;
#endif

		clippingPlaneRenderer = new PlaneRenderer(device, wgpu::TextureFormat(window->swapPref), camera);
		clippingPlaneRenderer->resize(width, height);

		// setup volume renderer
		printf("Setup volume renderer...\n");
		volumeRenderer = new VolumeRenderer();
		volumeRenderer->setClippingPlaneRenderer(clippingPlaneRenderer);

		volumeRenderer->setCamera(camera);
		volumeRenderer->setWidth(width);
		volumeRenderer->setHeight(height);
		volumeRenderer->setDevice(device);
		volumeRenderer->setPreferredSwapChainTextureFormat(wgpu::TextureFormat(window->swapPref));
#ifndef __EMSCRIPTEN__
		volumeRenderer->setupImGui(win);
#endif

		annotationHandler = new AnnotationHandler(device, camera, width, height);
		volumeRenderer->setAnnotationHandler(annotationHandler);

		std::string file("data/320x320x448/config.json");
		//std::string file("work/data/nv/ts_16.json");
		//volumeRenderer->openVolume(file);

#ifndef __EMSCRIPTEN__
		fileWatchHelper = new FileWatchHelper();
		fileWatchHelper->volumeRenderer = volumeRenderer;
		fileWatchHelper->annotationHandler = annotationHandler;
		fileWatchHelper->planeRenderer = clippingPlaneRenderer;
#endif

		glm::vec4 clippingPlaneOrigin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0);
		glm::vec4 clippingPlaneNormal = glm::vec4(glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)), 0.0f);

		annotationHandler->clippingPlaneOrigin = clippingPlaneOrigin;
		annotationHandler->clippingPlaneNormal = clippingPlaneNormal;

		volumeRenderer->setClippingPlaneOrigin(clippingPlaneOrigin);
		volumeRenderer->setClippingPlaneNormal(clippingPlaneNormal);

		clippingPlaneRenderer->clippingPlaneOrigin = clippingPlaneOrigin;
		clippingPlaneRenderer->clippingPlaneNormal = clippingPlaneNormal;

		// show the window & run the main loop
		window->show(wHnd);
		window->loop(wHnd, render);
		//int width, height;
		//glfwGetFramebufferSize(window->_window, &width, &height);
		//resizeHandler(width, height);

#ifndef __EMSCRIPTEN__
		delete camera;
		delete cameraController;
		delete volumeRenderer;
		delete annotationHandler;
		// destroy the window
		window->destroy(wHnd);
#endif	
	}
}

/**
  * Main application entry point.
  */
int main(int /*argc*/, char* /*argv*/[])
{
#ifndef __EMSCRIPTEN__
	startApp();
#endif

	return 0;
}


std::string rgb2hex(int r, int g, int b, bool with_head)
{
	std::stringstream ss;
	if (with_head)
		ss << "#";
	ss << std::hex << (r << 16 | g << 8 | b);
	return ss.str();
}


// =================== "API to JS" =====================
#ifdef __EMSCRIPTEN__
/**
  * JS exposed handle function that allows to call "Module.showImGui(bool)" from the web app.
  */
void showImGui(bool state) {
	//renderer->showImGui(state);
}

/**
  * JS exposed handle function that allows to call "Module.setColor(r,g,b)" from the web app.
  */
void setColor(float r, float g, float b) 
{
	volumeRenderer->setClearColor(r, g, b);
}

void stream_volume(const std::string &fn)
{
	std::cout << "";
}

std::string read_file(const std::string &fn)
{
	std::ifstream f(fn);
	std::string line;
	std::getline(f, line);
	return line;
}

void enableAO(bool state) {
	volumeRenderer->enableAO(state);
}

void chooseClippingPlane(int state) {
	volumeRenderer->setClippingPlaneModus(state);
}

void clipVolume(bool enable, int volume)
{
	switch (volume)
	{
	case 0:
		volumeRenderer->setClippingMaskA(enable);
		break;
	case 1:
		volumeRenderer->setClippingMaskB(enable);
		break;
	case 2:
		volumeRenderer->setClippingMaskC(enable);
		break;
	case 3:
		break;
	default:
		break;
	}
}

void update_mask(int which)
{
	volumeRenderer->updateSegmentationMask(which, "new_mask.raw");
	emscripten_run_script("var loadingGif = document.getElementById('loadingcontainer'); loadingcontainer.style.display = 'none';");
}

void openVolume()
{
	std::string file = "config.json";
	volumeRenderer->openVolume(file);

	auto volume_descriptors = volumeRenderer->getVolumeDescriptors();
	auto tf0 = volume_descriptors[0].transferFunction;
	auto tf1 = volume_descriptors[1].transferFunction;
	auto tf2 = volume_descriptors[2].transferFunction;

	// disable the loading gif
	emscripten_run_script("var loadingGif = document.getElementById('loadingcontainer'); loadingcontainer.style.display = 'none';");

	// update the interface values of the browser
	std::string script = "document.getElementById('tf0_ramp_low').value = " + std::to_string(tf0.rampLow * 100.0f) + ";";
	//std::cout << script << std::endl;
	emscripten_run_script(script.c_str());
	script = "document.getElementById('tf1_ramp_low').value = " + std::to_string(tf1.rampLow * 100.0f) + ";";
	emscripten_run_script(script.c_str());
	//std::cout << script << std::endl;
	script = "document.getElementById('tf2_ramp_low').value = " + std::to_string(tf2.rampLow * 100.0f) + ";";
	emscripten_run_script(script.c_str());
	//std::cout << script << std::endl;
	
	std::string color0 = "document.getElementById('tf0_color').value = '";
	color0 += rgb2hex(tf0.color.r * 255.0f, tf0.color.g * 255.0f, tf0.color.b * 255.0f, true) + "';";
	std::string color1 = "document.getElementById('tf1_color').value = '";
	color1 += rgb2hex(tf1.color.r * 255.0f, tf1.color.g * 255.0f, tf1.color.b * 255.0f, true) + "';";
	std::string color2 = "document.getElementById('tf2_color').value = '";
	color2 += rgb2hex(tf2.color.r * 255.0f, tf2.color.g * 255.0f, tf2.color.b * 255.0f, true) + "';";

	emscripten_run_script(color0.c_str());
	emscripten_run_script(color0.c_str());
	emscripten_run_script(color1.c_str());
	emscripten_run_script(color2.c_str());
}

void setAnnotationKernelSize(int size)
{
	volumeRenderer->setAnnotationKernelSize(size);
}

void adjustTransferFunction(int which, float lowRamp, float highRamp, float red, float blue, float green)
{
	glm::vec3 color(red, green, blue);
	volumeRenderer->setTransferFunction(which, lowRamp, highRamp, color);
}

void enable_volume(int which)
{
	switch (which)
	{
	case 0:
		volumeRenderer->toggleVolumeA(); 
		break;
	case 1:
		volumeRenderer->toggleVolumeB();
		break;
	case 2:
		volumeRenderer->toggleVolumeC();
		break;
	case 3:
		break;
	default:	
		break;
	}
}

void save_annotations()
{
	annotationHandler->serialize("annotations.json");
}

/**void loadTransferFunction(std::string fileName)
{
	TransferFunction tf = volumeRenderer->loadTransferFunction(fileName);
	//std::cout << fileName << std::endl;
	//emscripten_run_script("alert('TF loaded.')");
}**/

EMSCRIPTEN_BINDINGS(my_module) 
{
	function("save_annotations", &save_annotations);
	function("stream_volume", &stream_volume);
	function("enable_volume", &enable_volume);
	function("start_app", &startApp);
	function("update_mask", &update_mask);
	function("read_file", &read_file);
	function("setAnnotationKernelSize", &setAnnotationKernelSize);
	function("adjustTransferFunction", &adjustTransferFunction);
	function("open_volume", &openVolume);
	function("clip_volume", &clipVolume);
	function("chooseClippingPlane", &chooseClippingPlane);
	//function("loadTransferFunction", &loadTransferFunction);
	function("enableAO", &enableAO);
	function("showImGui", &showImGui);
	function("setColor", &setColor);
}
#endif // __EMSCRIPTEN__
// =================== "API to JS" END =====================

