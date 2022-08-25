# voxels

Voxels is a real-time voxel path tracer, using the Vulkan API.
It is being developed as a senior project at Kansas State University.

## Building

This project uses CMake for building.
The easiest way to run it would be to open the project in CLion, or another C/C++ IDE which supports CMake.

Dependencies are managed using [Conan](https://conan.io/), which requires a separate installation.
You'll also want the [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) for shader compilation.
