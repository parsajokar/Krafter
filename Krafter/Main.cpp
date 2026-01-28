#include <memory>

#include "Krafter/Game.h"

int main(int argc, char** argv)
{
    const Krafter::ApplicationSpecification specification = {
        .name = "Krafter"
    };

    std::make_unique<Krafter::GameApplication>(specification)->Run();

    return 0;
}
