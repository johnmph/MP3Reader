#ifndef MP3_FRAME_DATA_SIDEINFORMATION_HPP
#define MP3_FRAME_DATA_SIDEINFORMATION_HPP

#include <array>


namespace MP3::Frame::Data {

    constexpr std::array<std::array<unsigned int, 2>, 16> scaleFactorCompress = {{
        { 0, 0 },
        { 0, 1 },
        { 0, 2 },
        { 0, 3 },
        { 3, 0 },
        { 1, 1 },
        { 1, 2 },
        { 1, 3 },
        { 2, 1 },
        { 2, 2 },
        { 2, 3 },
        { 3, 1 },
        { 3, 2 },
        { 3, 3 },
        { 4, 2 },
        { 4, 3 }
    }};

    constexpr std::array<float, 2> scaleFactorScale = {
        0.5f,
        1.0f
    };

}

#endif /* MP3_FRAME_DATA_SIDEINFORMATION_HPP */
