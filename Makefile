#
# Makefile to use with emscripten
# See https://emscripten.org/docs/getting_started/downloads.html
# for installation instructions.
#
# This Makefile assumes you have loaded emscripten's environment.
# (On Windows, you may need to execute emsdk_env.bat or encmdprompt.bat ahead)
#
# Running `make` will produce three files:
#  - web/index.html (current stored in the repository)
#  - web/index.js
#  - web/index.wasm
#
# All three are needed to run the demo.

CC = emcc
CXX = em++ --bind 
# This file system provides read-only access to File and Blob objects inside a worker 
# without copying the entire data into memory and can potentially be used for huge files. 
# https://emscripten.org/docs/api_reference/Filesystem-API.html#filesystem-api-workerfs
CXX += -lworkerfs.js
WEB_DIR = out/web
EXE = $(WEB_DIR)/index.js
INC_DIR = inc
SRC_DIR = src
UTIL_DIR = $(SRC_DIR)/dawn/utils
COMMON_DIR = $(SRC_DIR)/dawn/common
GLM_DIR = lib/glm
SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/AnnotationHandler.cpp $(SRC_DIR)/VolumeRenderer.cpp $(SRC_DIR)/Helper.cpp
SOURCES += $(SRC_DIR)/Camera.cpp $(SRC_DIR)/CameraController.cpp $(SRC_DIR)/Hotkeys.cpp $(SRC_DIR)/Input.cpp $(SRC_DIR)/PagedVolume.cpp 
SOURCES += $(SRC_DIR)/PlaneRenderer.cpp $(SRC_DIR)/SegmentationUpdateHandler.cpp $(SRC_DIR)/ShaderUtil.cpp $(SRC_DIR)/RendererWindow.cpp $(SRC_DIR)/VolumeLoader.cpp
SOURCES += $(SRC_DIR)/dawn/utils/ComboRenderPipelineDescriptor.cpp $(SRC_DIR)/dawn/utils/TextureUtils.cpp $(SRC_DIR)/dawn/utils/WGPUHelpers.cpp 
SOURCES += $(SRC_DIR)/dawn/common/Assert.cpp $(SRC_DIR)/dawn/common/Log.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
#UNAME_S := $(shell uname -s)

##---------------------------------------------------------------------
## EMSCRIPTEN OPTIONS
##---------------------------------------------------------------------
 
#EMS += -fsanitize=address
EMS += -s USE_GLFW=3 -s USE_WEBGPU=1 -s WASM=1
#EMS += -s "EXPORTED_FUNCTIONS=['_loadFile', '_main']"
#EMS += -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap']"
EMS += -s "EXTRA_EXPORTED_RUNTIME_METHODS=['FS']"
EMS += -s FORCE_FILESYSTEM=1
EMS += -s ALLOW_MEMORY_GROWTH=1
EMS += -s DISABLE_EXCEPTION_CATCHING=0 -s NO_EXIT_RUNTIME=1
EMS += -s ASSERTIONS=0
# optimization flags
# -O0 No optimizations (default).
# -O3 Like -O2, but with additional optimizations that may take longer to run. Good setting for a release build.
EMS += -O3
# Building with -sEVAL_CTORS will evaluate as much code as possible at compile time. 
EMS += -sEVAL_CTORS
#EMS += -s SAFE_HEAP=1

#INITIAL_MEMORY should be larger than TOTAL_STACK
EMS += -s INITIAL_MEMORY=1024MB
EMS += -s TOTAL_STACK=512MB
#EMS += --shell-file shell_kaust.html

# Emscripten allows preloading a file or folder to be accessible at runtime.
# The Makefile for this example project suggests embedding the misc/fonts/ folder into our application, it will then be accessible as "/fonts"
# See documentation for more details: https://emscripten.org/docs/porting/files/packaging_files.html
# (Default value is 0. Set to 1 to enable file-system and include the misc/fonts/ folder as part of the build.)
#USE_FILE_SYSTEM ?= 0
#ifeq ($(USE_FILE_SYSTEM), 0)
#EMS += -s NO_FILESYSTEM=1 -DIMGUI_DISABLE_FILE_FUNCTIONS
#endif
#ifeq ($(USE_FILE_SYSTEM), 1)
#LDFLAGS += --no-heap-copy --preload-file ../../misc/fonts@/fonts 
#endif
#LDFLAGS += -s LLD_REPORT_UNDEFINED
LDFLAGS += --preload-file work/@/work

##---------------------------------------------------------------------
## FINAL BUILD FLAGS
##---------------------------------------------------------------------

CPPFLAGS = -I$(INC_DIR)
CPPFLAGS += -I$(GLM_DIR)
CPPFLAGS += -I$(SRC_DIR)
CPPFLAGS += -I$(UTIL_DIR)

# -I$(IMGUI_DIR) 
#-I$(IMGUI_DIR)/backends
#CPPFLAGS += -g
CPPFLAGS += -Wall -Wformat -Os
CPPFLAGS += $(EMS)
LIBS += $(EMS)
#LDFLAGS += --shell-make`file shell_minimal.html

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:$(EMS_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o:$(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
	
%.o:$(UTIL_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
	
%.o:$(COMMON_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<


all: $(EXE)
	@echo Build complete for $(EXE)

$(WEB_DIR):
	mkdir $@

serve: all
	python3 -m http.server -d $(WEB_DIR)

$(EXE): $(OBJS) $(WEB_DIR)
	$(CXX) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

clean:
	rm -f $(EXE) $(OBJS) $(WEB_DIR)/index.js $(WEB_DIR)/*.wasm $(WEB_DIR)/*.wasm.pre
