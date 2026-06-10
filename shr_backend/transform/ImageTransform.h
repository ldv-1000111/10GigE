#pragma once
#include <vector>
#include <cstdint>
#include "VmbC/VmbC.h"
#include "VmbImageTransform/VmbTransform.h"

namespace ImageTransform {
    bool bayerToRGB8(VmbUchar_t*        pSrc,
                     VmbPixelFormat_t   srcFormat,
                     VmbUint32_t       width,
                     VmbUint32_t       height,
                     std::vector<uint8_t>& outRGB);
}
