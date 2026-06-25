#pragma once

#include "Krafter/Core/Application.h"

namespace Krafter {

// The application composition root: assembles the scene layers it runs and
// wires up the references they share.
class GameApplication : public Application {
public:
    GameApplication(const ApplicationSpecification& specification);
};

} // namespace Krafter
