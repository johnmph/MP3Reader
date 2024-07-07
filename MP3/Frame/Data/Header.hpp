#ifndef MP3_FRAME_DATA_HEADER_HPP
#define MP3_FRAME_DATA_HEADER_HPP

#include <array>


namespace MP3::Frame::Data {

    constexpr std::array<unsigned int, 16> bitrates = {
        0,
        32,
        40,
        48,
        56,
        64,
        80,
        96,
        112,
        128,
        160,
        192,
        224,
        256,
        320,
        static_cast<unsigned int>(-1)
    };

    constexpr std::array<unsigned int, 4> samplingRates = {
        44100,
        48000,
        32000,
        static_cast<unsigned int>(-1)
    };

}

#endif /* MP3_FRAME_DATA_HEADER_HPP */
