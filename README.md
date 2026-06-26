# Krafter

Krafter is a **Minecraft-style voxel sandbox game** built from scratch in **C++20** and **OpenGL 4.5**.

[demo1.webm](https://github.com/user-attachments/assets/baaa82f7-d113-42d4-9c09-26da6cfa86e8)

[demo2.webm](https://github.com/user-attachments/assets/060aa9d1-d969-4858-b85e-783ee43684b3)

## 🛠️ Tech Stack

- **Language:** C++20
- **Graphics:** OpenGL 4.5
- **Libraries:**
  - GLFW — Windowing & Input
  - GLAD — OpenGL 4.5 Functionalities
  - ImGui — Immediate Mode GUI
  - GLM — Math Library with SIMD Instruction Support
  - stb — Texture File Loading Library
  - FastNoiseLite — Noise Generation Library

## 📦 Building Krafter

### Requirements

- C++20 compiler
- CMake 3.20+
- Ninja 1.12+
- OpenGL 4.5 capable GPU

### Clone the repository

```bash
git clone --recursive https://github.com/parsajokar/Krafter.git
cd Krafter
mkdir build
cd build
cmake -G "Ninja Multi-Config" ..
cd ..
cmake --build build/ --config RelWithDebInfo
./build/RelWithDebInfo/Krafter
```

### Controls
 - `Space` — Toggling `Viewport Mode` and `Configuration Mode`
 - `F11` — Toggling `Windowed Mode` and `Fullscreen Mode`
 - `1–9 + 0` — Selecting hotbar slots
 - `WASD + Mouse` — Moving the camera in `Viewport Mode`
