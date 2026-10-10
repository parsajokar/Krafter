#include <memory>

#include "Krafter/Game/GameApplication.h"

int main(int, char**)
{
    const Krafter::ApplicationSpecification specification = {
        .name = "Krafter",
        .workingDirectory = "",
    };

    std::make_unique<Krafter::GameApplication>(specification)->Run();

    return 0;
}
