# Project 2: OpenGL Scenes

This directory contains three applications demonstrating advanced OpenGL rendering techniques using the EnGene library.

## Applications

### 1. proj2.cpp - Table Scene
A static scene featuring:
- Table with four legs
- Sphere with environment reflection, roughness mapping, and clip plane
- Cylinder object
- Articulated lamp with spotlight
- Directional lighting (sun/ambient)
- Fog effect
- Diffuse textures

### 2. proj2-2.cpp - Solar System
An animated solar system featuring:
- Sun with emissive shader and point light
- Mercury with orbit animation
- Earth with orbit and rotation, plus roughness mapping
- Moon orbiting Earth
- Starfield skybox
- Camera switching (global view / Earth-based view)

### 3. proj2-mix.cpp - Combined Scenes (Recommended)
Combines both scenes with runtime switching:
- Press '1' to view Table Scene
- Press '2' to view Solar System Scene
- Press 'C' to switch cameras (Solar System only)
- All features from both scenes included

## Building

### Windows (MinGW)
```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

### Linux
```bash
cmake -B build
cmake --build build
```

## Running

From the `test/proj2` directory:

```bash
# Table Scene
./build/Proj2Table.exe

# Solar System
./build/Proj2Solar.exe

# Mixed Scenes (Recommended)
./build/Proj2Mix.exe
```

## Controls

### All Scenes
- **Left Mouse Button + Drag**: Rotate camera (orbit)
- **Middle Mouse Button + Drag**: Pan camera
- **Mouse Wheel**: Zoom in/out
- **ESC**: Exit

### Mixed Scene (proj2-mix)
- **'1' Key**: Switch to Table Scene
- **'2' Key**: Switch to Solar System Scene
- **'C' Key**: Switch cameras (Solar System only - Global/Earth view)

## Features Demonstrated

### Base Requirements
- ✅ Fragment lighting (Blinn-Phong)
- ✅ Multiple light types (Directional, Point, Spot)
- ✅ Diffuse textures
- ✅ Scene graph hierarchy
- ✅ Arcball camera controls

### Extra Features
- ✅ Fog (exponential squared)
- ✅ Clip planes (sphere in table scene)
- ✅ Roughness mapping (affects shininess)
- ✅ Environment mapping (cubemap reflection)
- ✅ Skybox rendering
- ✅ Animated scene graph (solar system)
- ✅ Multiple cameras with switching
- ✅ Emissive shader (sun)

## Shader Files

Located in `shaders/` directory:
- `phong.vert` / `phong.frag` - Main Blinn-Phong lighting shader
- `sun.vert` / `sun.frag` - Emissive shader for the sun

## Notes

- Textures are procedurally generated (no external image files required)
- Cubemaps are also procedurally generated
- The mixed scene uses node applicability to show/hide scene branches
- Light manager automatically updates when switching scenes
- All paths are relative to the EnGene repository structure
