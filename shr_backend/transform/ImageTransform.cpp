#include "ImageTransform.h"
#include <QDebug>

namespace ImageTransform {

bool bayerToRGB8(VmbUchar_t*        pSrc,
                 VmbPixelFormat_t   srcFormat,
                 VmbUint32_t       width,
                 VmbUint32_t       height,
                 std::vector<uint8_t>& outRGB)
{
    VmbImage src{}, dst{};
    src.Size = sizeof(src);
    dst.Size = sizeof(dst);
    src.Data = pSrc;
    dst.Data = outRGB.data();

    VmbError_t err = VmbSetImageInfoFromPixelFormat(srcFormat, width, height, &src);
    if (err != VmbErrorSuccess) {
        qWarning() << "VmbSetImageInfoFromPixelFormat failed:" << err;
        return false;
    }

    err = VmbSetImageInfoFromInputImage(&src, VmbPixelLayoutRGB, 8, &dst);
    if (err != VmbErrorSuccess) {
        qWarning() << "VmbSetImageInfoFromInputImage failed:" << err;
        return false;
    }

    VmbTransformInfo debayer{};
    VmbSetDebayerMode(VmbDebayerMode2x2, &debayer);

    err = VmbImageTransform(&src, &dst, &debayer, 1);
    if (err != VmbErrorSuccess) {
        qWarning() << "VmbImageTransform failed:" << err;
        return false;
    }

    return true;
}

} // namespace ImageTransform
