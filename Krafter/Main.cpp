#include "Krafter/Game.h"

int main(int argc, char** argv)
{
    const Krafter::ApplicationSpec spec = {
        .name = "Krafter"
    };

    auto app = new Krafter::GameApplication(spec);
    app->Run();
    delete app;

    return 0;
}
