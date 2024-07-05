#ifndef MP3_FRAME_DATA_FRAME_HPP
#define MP3_FRAME_DATA_FRAME_HPP

#include <array>


namespace MP3::Frame::Data {
    
    constexpr std::array<unsigned int, 5> scaleFactorBandsPerScaleFactorShareGroup = {
        0,
        6,
        11,
        16,
        21
    };

    constexpr std::array<unsigned int, 21> pretabByScaleFactorBand = {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        1,
        1,
        1,
        2,
        2,
        3,
        3,
        3,
        2
    };

}

#endif /* MP3_FRAME_DATA_FRAME_HPP */
