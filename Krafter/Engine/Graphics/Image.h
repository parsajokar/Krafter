#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace Krafter {

struct Image {
    struct PixelDeleter {
        void operator()(uint8_t* data) const;
    };

    int32_t width = 0;
    int32_t height = 0;
    std::unique_ptr<uint8_t, PixelDeleter> pixels;

    explicit operator bool() const
    {
        return pixels != nullptr;
    }
};

Image LoadImage(const std::string& path, bool flipVertically);

}
