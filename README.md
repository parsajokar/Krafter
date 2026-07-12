# Krafter

Krafter is a **Minecraft-style voxel sandbox game** built from scratch in **C++20** and **Vulkan**.

[demo1.webm](https://github.com/user-attachments/assets/baaa82f7-d113-42d4-9c09-26da6cfa86e8)

[demo2.webm](https://github.com/user-attachments/assets/060aa9d1-d969-4858-b85e-783ee43684b3)

## ✨ Features

- **Multithreaded chunk streaming** — a custom thread-pool job system runs a
  three-stage asynchronous pipeline (terrain generation → sky-light → meshing),
  marshalling finished work back to the main thread through thread-safe result
  queues. Chunks load and unload around the player by render distance, with mesh
  uploads capped per frame to keep the frame time smooth.
- **Procedural world generation** — fully seedable worlds built from a multi-noise
  climate model (temperature, humidity, continentalness) that selects between five
  biomes (ocean, oak forest, birch forest, savannah, desert) and blends their
  terrain heights so coastlines and borders slope smoothly instead of cliffing.
  Populated with trees, grass, ferns, dead bushes, cacti, and lakes at sea level.
- **Custom voxel mesher** — per-chunk meshing with hidden-face culling, **ambient
  occlusion**, and **smooth lighting** that samples the full 3×3 chunk neighbourhood
  (including diagonals across borders) for seamless shading. Separate opaque,
  cutout (cross-shaped plants), and transparent (water) render passes, with
  biome-tinted grass and foliage.
- **Sky-light propagation** — a flood-fill lighting pass computes light levels in
  `[0, 15]` with a one-chunk apron so values match exactly at chunk borders, with
  no seams.
- **Dynamic day/night cycle** — a running time-of-day drives the sun direction,
  sun colour, ambient fill, and sky colour, feeding directional + ambient lighting
  in the shaders.
- **Two gameplay modes** — *Survive* with AABB block collision, gravity, and
  jumping, and *Spectate* free-flight noclip.
- **Block interaction** — a voxel DDA raycast targets blocks for breaking and
  placing (with placement rules for plants and self-trap prevention), including
  hold-to-break and hold-to-place. Breaking is **gradual**, advancing a crack
  overlay over each block's mining time, and a chopped tree's stranded wood and
  leaves crumble away.
- **Mining, tools & drops** — breaking a block requires the right tool, decided by
  per-block **harvest tags** (axe / pickaxe / shovel, combinable), with a wooden
  axe for wood and foliage. Broken blocks spawn collectible item drops that fall,
  settle, and are picked up into the inventory.
- **Inventory & crafting** — a full-screen inventory (a 3×10 storage grid over the
  hotbar row) with Minecraft-style click-to-pick-up/drop/stack item handling, and a
  **Terraria-style crafting bar**: a horizontally scrolling, edge-fading strip of
  recipes that glides the clicked recipe into a fixed selection frame, previews its
  ingredients beneath with an arrow, supports hold-to-craft, and lists only the
  recipes you have materials for.
- **Data-driven blocks** — every block's render, occlusion, mining, drop, and
  harvest-tool data lives in one per-block table, so adding or retuning a block is a
  single entry rather than edits scattered across the codebase.
- **From-scratch rendering & UI** — a Vulkan renderer, a custom bitmap-font
  text renderer, a 9-slice sprite UI (main menu with seed input, pause menu, hotbar
  HUD, inventory & crafting screen, inverted-blend crosshair), and an ImGui debug
  overlay.
- **Layered engine architecture** — an application/layer-stack design with an event
  system and deferred layer operations for safe scene transitions.

## 🛠️ Tech Stack

- **Language:** C++20
- **Graphics:** Vulkan
- **Libraries:**
  - GLFW — Windowing & Input
  - Vulkan Memory Allocator (VMA) — GPU Memory Management
  - ImGui — Immediate Mode GUI
  - GLM — Math Library with SIMD Instruction Support
  - stb — Texture File Loading Library
  - FastNoiseLite — Noise Generation Library

## 📦 Building Krafter

### Requirements

- C++20 compiler
- CMake 3.20+
- Ninja 1.12+
- Vulkan SDK (provides `glslc` for shader compilation)
- Vulkan 1.0 capable GPU

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

## 🎮 Gameplay

From the main menu you enter a seed (any text or number, or leave it blank for a
random world) and choose one of two modes:

- **Survive** — Minecraft-style physics: gravity, block collision, and jumping.
- **Spectate** — Free flight in the look direction, with no gravity or collision.

### Controls

Shared:
 - `WASD` — Move
 - `Mouse` — Look around
 - `Left Click` — Break the targeted block (hold to keep mining; needs the right tool)
 - `Right Click` — Place the selected block (hold to keep placing)
 - `1–9 + 0` — Select hotbar slot
 - `E` — Open / close the inventory and crafting screen
 - `Esc` — Open the pause menu (Resume / Exit to main menu)
 - `F3` — Toggle the debug overlay (movement speed, mouse sensitivity, field of view, and more)
 - `F11` — Toggle `Windowed Mode` and `Fullscreen Mode`

Survive only:
 - `Space` — Jump (hold to keep jumping)

In the inventory screen:
 - `Left Click` a slot — Pick up, drop, or stack items on the cursor
 - `Left Click` a recipe — Scroll it into the selection frame; click the selected
   recipe to craft it (hold to keep crafting). The result lands on the cursor.
 - `Mouse Wheel` — Move the crafting selection between recipes
