#include "Krafter/Game.h"
#include "Krafter/UILayer.h"
#include "Krafter/WorldLayer.h"

namespace Krafter {

GameApplication::GameApplication(const ApplicationSpecification& specification)
    : Application(specification)
{
    WorldLayer* world = new WorldLayer(GetWindow(), GetRenderer());
    PushLayer(world);

    // The HUD shares the world's player hotbar. The overlay is pushed after the
    // world layer and so is destroyed before it, keeping the reference valid.
    PushOverlay(new UILayer(GetWindow(), world->GetHotbar()));
}

} // namespace Krafter
