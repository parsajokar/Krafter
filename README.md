# Krafter

Krafter is a **Minecraft-style voxel sandbox game** built from scratch in **C++20** and **Vulkan**.

[demo1.webm](https://github.com/user-attachments/assets/baaa82f7-d113-42d4-9c09-26da6cfa86e8)

[demo2.webm](https://github.com/user-attachments/assets/060aa9d1-d969-4858-b85e-783ee43684b3)

## ✨ Features

- **Multithreaded chunk streaming** — a custom thread-pool job system runs a
  three-stage pipeline (terrain generation → lighting → meshing) on worker
  threads. Workers read copy-on-write snapshots of chunks, so the main thread can
  keep editing while they work, and every result is versioned so stale work is
  never applied over newer changes. Chunks load and unload around the player by
  render distance, with first-time mesh uploads capped per second to keep the
  frame time smooth.
- **Edits survive unloading** — chunks you change are kept in memory when they
  unload and come back exactly as you left them, flowing water included (saving
  to disk isn't implemented yet).
- **Compact chunk storage** — each 16×256×16 column is stored as 16 sections of
  16³; all-air sections take no memory, blocks are 2 bytes, and sky and block
  light share a byte, with uniform light sections (open sky, solid rock) stored as
  a single value.
- **Procedural world generation** — fully seedable worlds built from a multi-noise
  climate model (temperature, humidity, continentalness) that selects between five
  biomes (ocean, oak forest, birch forest, savannah, desert) and blends their
  terrain heights so coastlines and borders slope smoothly instead of cliffing.
  Below ground: caves with lava pools deep down, aquifers, coal, copper and iron
  ores, and hollow ice-shelled geodes lined with topaz, emerald, amethyst and
  diamond. On the surface: oak, birch and acacia trees, grass, ferns, flowers,
  dead bushes, cacti, lakes, and red-sand claw marks across the deserts.
- **Lighting** — sky light and block light (torches, lava, furnaces) in `[0, 15]`.
  A chunk's first light is a full flood fill with a one-chunk apron, so values
  match exactly at borders; after that, edits update light incrementally, touching
  only the cells whose light actually changes and remeshing only the chunks that
  can see them.
- **Custom voxel mesher** — per-chunk meshing with hidden-face culling, **ambient
  occlusion**, and **smooth lighting** that samples the full 3×3 chunk
  neighbourhood (including diagonals across borders) for seamless shading.
  Vertices are packed into 20 bytes in chunk-local coordinates. Separate opaque,
  cutout (plants and gems), translucent (ice), and water passes, with biome-tinted
  grass and foliage, sloped flowing-water surfaces, and animated water and lava.
- **GPU-driven chunk rendering** — chunk geometry lives in a few large shared GPU
  buffers; each pass binds its pipeline once and draws every visible chunk with
  multi-draw indirect (with a plain-draw fallback). Chunks are frustum-culled
  against their actual height, opaque geometry is drawn front to back and
  transparent geometry back to front, and everything is rendered relative to the
  camera so the world stays precise far from the origin.
- **Fixed-timestep simulation** — the world simulates at 60 ticks per second
  regardless of frame rate, with the camera and item drops interpolated between
  ticks so movement stays smooth on any display.
- **Block behaviours & fluids** — a block-update queue drives per-block
  behaviours: plants and torches pop off when their support is broken, cacti
  topple, gems break away from their wall, a chopped tree's stranded wood and
  leaves crumble away, and water and lava flow and spread, turning to stone where
  they meet.
- **Dynamic day/night cycle** — a running time-of-day drives the sun direction,
  sun colour, ambient fill, and sky colour, feeding directional + ambient lighting
  in the shaders.
- **Two gameplay modes** — *Survive* with AABB block collision, gravity, and
  jumping, and *Spectate* free-flight noclip.
- **Block interaction** — a voxel DDA raycast targets blocks for breaking and
  placing (with placement rules for plants and self-trap prevention), including
  hold-to-break and hold-to-place. Breaking is **gradual**, advancing a crack
  overlay over each block's mining time.
- **Mining, tools & drops** — wooden, stone, copper, and iron axes, pickaxes,
  shovels, and swords. Each block needs the right tool type and tier, and better
  tools mine faster. Broken blocks spawn item drops that fall, settle, and fly to
  the player when there's room in the inventory; otherwise they stay on the
  ground.
- **Inventory & crafting** — a full-screen inventory (a 3×10 storage grid over the
  hotbar row) with Minecraft-style click-to-pick-up/drop/stack item handling, and a
  **Terraria-style crafting bar**: a horizontally scrolling, edge-fading strip of
  recipes that glides the clicked recipe into a fixed selection frame, previews its
  ingredients beneath with an arrow, supports hold-to-craft, and lists only the
  recipes you have materials for. Some recipes need a nearby **workbench**, and a
  **furnace** smelts ores into ingots.
- **Data-driven content** — every block's render, lighting, mining, drop, and
  harvest-tool data lives in one per-block table; every item (blocks, tools,
  materials) shares one item registry; and block behaviours are entries in a
  behaviour table, so adding or retuning content is a single entry rather than
  edits scattered across the codebase.
- **From-scratch rendering & UI** — a Vulkan renderer, a custom bitmap-font
  text renderer, a 9-slice sprite UI (main menu with seed input, pause menu, hotbar
  HUD, inventory & crafting screen, inverted-blend crosshair), and an ImGui debug
  overlay.
- **Layered engine architecture** — an application/layer-stack design with an event
  system and deferred layer operations for safe scene transitions. The world
  simulation has no dependency on rendering, so it runs headless.

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

## 🗂️ Project Structure

Each top-level folder only depends on the ones listed above it.

```
Krafter/
├── Engine/           Application, layers, events, window, job system, fixed timestep
│   └── Graphics/     Vulkan backend: device, swapchain, buffers, textures, pipelines
├── World/            Chunks, terrain generation, lighting, block behaviours, fluids, items
├── Rendering/        World and chunk rendering, meshing, camera, UI renderer, font
├── Gameplay/         Player, inventory, hotbar, crafting
├── UI/               Main menu, pause menu, inventory screen, HUD
└── Game/             The game application and world layer
assets/
├── shaders/          GLSL shaders, compiled to SPIR-V at build time
└── textures/         Block, item, UI, and font atlases
```

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
 - `F3` — Toggle the debug overlay in-game (movement speed, render distance, chunk and memory stats, and more)
 - `F11` — Toggle `Windowed Mode` and `Fullscreen Mode`

Survive only:
 - `Space` — Jump (hold to keep jumping)

In the inventory screen:
 - `Left Click` a slot — Pick up, drop, or stack items on the cursor
 - `Left Click` a recipe — Scroll it into the selection frame; click the selected
   recipe to craft it (hold to keep crafting). The result lands on the cursor.
 - `Mouse Wheel` — Move the crafting selection between recipes
 - Closing the inventory puts the held stack back; anything that doesn't fit is
   dropped in front of you.
