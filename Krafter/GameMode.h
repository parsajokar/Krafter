#pragma once

namespace Krafter {

// How the world plays, chosen on the main menu. Survival gives the player
// Minecraft-style physics (gravity, block collision, jumping); Spectator keeps
// the free-flying noclip camera used while building and exploring.
enum class GameMode {
    k_Survival,
    k_Spectator,
};

} // namespace Krafter
