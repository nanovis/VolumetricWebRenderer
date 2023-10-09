# WebRenderer

Volume Rendering project combining WebGPU, Dawn, ImGui and Emscripten. It can be built using VS (for desktop) or using the make file (for web deployment).

# Inspired by (and derived from) the following examples
- https://github.com/cwoffenden/hello-webgpu
- https://github.com/ocornut/imgui/tree/master/examples/example_emscripten_wgpu

# How to build
- Desktop
  - Open Visual Studio solution (tested 2019) and build. The output should be in `out/x64/Debug/` and `out/x64/Release/`.
- Web
  - You need to install Emscripten from https://emscripten.org/docs/getting_started/downloads.html, and have the environment variables set, as described in https://emscripten.org/docs/getting_started/downloads.html#installation-instructions
  - Depending on your configuration, in Windows you may need to run emsdk/emsdk_env.bat in your console to access the Emscripten command-line tools.
  - Open commandline and type `make`. The output should be in `out/web/`.

# How to use
- left mouse button: camera rotation
- right mouse button: changes clipping plane offset (hold right mouse button and move along y direction)
- middle mouse button: camera panning
- mouse scrolling: zoom in/out
- key 0: disable clipping
- key 1: clipping x-axis
- key 2: clipping y-axis
- key 3: clipping z-axis
- key 4: clipping view-aligned
- key F: toggles the clipping plane into fullscreen mode
- key left-CTRL: Holding the left CTRL key activates the annotation mode, in which you can add or remove annotations. The annotations work only in combination with the clipping plane. You can use the following commands:
  - left mouse button: uses a spherical brush to add annotations to the selected volume mask
  - right mouse button: uses a spherical brush to remove annotations of the selected volume mask

The hotkeys can be changed by loading a hotkey mapping file. 

# Known issues:
- device lost when resizing window in the desktop mode
- web version currently works only when compiled with depth attachment disabled. This is due to an emscripten bug: https://github.com/emscripten-core/emscripten/issues/16471
