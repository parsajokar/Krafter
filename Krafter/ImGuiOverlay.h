#pragma once

#include "Krafter/Layer.h"

namespace Krafter {

class ImGuiOverlay : public Layer {
public:
    ImGuiOverlay();
    ~ImGuiOverlay() = default;

    void OnAttach() override;
    void OnDetach() override;

    void Begin();
    void End();
};

} // namespace Krafter
