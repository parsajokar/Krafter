# Krafter

Krafter is a **Minecraft-style voxel sandbox game** built from scratch in **C++20** and **OpenGL 4.5**.

[demo.webm](https://github.com/user-attachments/assets/5043914a-37fb-4729-80fd-e39bc23e5b1c)

## ✨ Features

- **Modern OpenGL 4.5 renderer**  
  Direct State Access (DSA), Debugging Callback, etc.
  
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
 - `WASD + Mouse` — Moving the camera in `Viewport Mode`
