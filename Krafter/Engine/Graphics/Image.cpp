#include <iostream>

#include "stb_image.h"

#include "Krafter/Engine/Graphics/Image.h"

namespace Krafter {

void Image::PixelDeleter::operator()(uint8_t* data) const
{
    stbi_image_free(data);
}

Image LoadImage(const std::string& path, bool flipVertically)
{
    stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);

    Image image;
    int channelsInFile = 0;
    image.pixels.reset(stbi_load(path.c_str(), &image.width, &image.height, &channelsInFile, STBI_rgb_alpha));
    if (!image) {
        std::cerr << "[FILE] Could not read " << path << ": " << stbi_failure_reason() << std::endl;
        image.width = 0;
        image.height = 0;
    }
    return image;
}

}
