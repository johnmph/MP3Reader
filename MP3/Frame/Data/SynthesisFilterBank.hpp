#ifndef MP3_FRAME_DATA_SYNTHESISFILTERBANK_HPP
#define MP3_FRAME_DATA_SYNTHESISFILTERBANK_HPP

#include <array>
#include <cmath>
#include "../../Helper.hpp"


namespace MP3::Frame::Data {

    // Butterfly coefficients
    constexpr std::array<float, 8> butterflyCoefficientCi = {
        -0.6f,
        -0.535f,
        -0.33f,
        -0.185f,
        -0.095f,
        -0.041f,
        -0.0142f,
        -0.0037f
    };

    constexpr auto butterflyCoefficientCs = Helper::transformContainer(butterflyCoefficientCi, [](auto value) { return 1.0f / (std::sqrt(1.0f + (value * value))); });
    constexpr auto butterflyCoefficientCa = Helper::transformContainer(butterflyCoefficientCi, [](auto value) { return value / (std::sqrt(1.0f + (value * value))); });

    // IMDCT
    template <typename TValue, int Size>
    constexpr std::array<TValue, Size * (Size / 2)> getIMDCTValues() {//TODO: constexpr mais ne peut pas l'etre car std::cos n'est pas constexpr
        std::array<TValue, Size * (Size / 2)> output = {};

        unsigned int const halfSize = Size / 2;

        for (unsigned int i = 0; i < Size; ++i) {
            for (unsigned int k = 0; k < halfSize; ++k) {
                output[(i * halfSize) + k] = std::cos((M_PI / (2.0f * Size)) * ((2.0f * i) + 1.0f + halfSize) * ((2.0f * k) + 1.0f));
            }
        }

        return output;
    }

    auto const imdctShortBlock = getIMDCTValues<float, 12>();//TODO: voir si utiliser ca ou laisser dans code, voir quand profiling
    auto const imdctLongBlock = getIMDCTValues<float, 36>();

    // Windowing
    using WindowingValues = std::array<float, 36>;

    constexpr WindowingValues getWindowingValuesBlock0() {//TODO: constexpr mais ne peut pas l'etre car std::sin n'est pas constexpr
        WindowingValues output = {};

        for (unsigned int i = 0; i < 36; ++i) {
            output[i] = std::sin((M_PI / 36.0f) * (i + 0.5f));
        }

        return output;
    }

    constexpr WindowingValues getWindowingValuesBlock1() {//TODO: constexpr mais ne peut pas l'etre car std::sin n'est pas constexpr
        WindowingValues output = {};

        for (unsigned int i = 0; i < 18; ++i) {
            output[i] = std::sin((M_PI / 36.0f) * (i + 0.5f));
        }

        for (unsigned int i = 18; i < 24; ++i) {
            output[i] = 1.0f;
        }

        for (unsigned int i = 24; i < 30; ++i) {
            output[i] = std::sin((M_PI / 12.0f) * (i - 17.5f)); // - 17.5f = - 18.0f + 0.5f
        }
        /*
        for (unsigned int i = 30; i < 36; ++i) {    // Already zero initialized so we don't need this
            output[i] = 0.0f;
        }*/

        return output;
    }

    constexpr WindowingValues getWindowingValuesBlock2() {//TODO: constexpr mais ne peut pas l'etre car std::sin n'est pas constexpr
        WindowingValues output = {};

        for (unsigned int i = 0; i < 36; ++i) {
            output[i] = std::sin((M_PI / 12.0f) * ((i % 12) + 0.5f));   // Mod 12 because it's 3 times the same (for each window)
        }

        return output;
    }

    constexpr WindowingValues getWindowingValuesBlock3() {//TODO: constexpr mais ne peut pas l'etre car std::sin n'est pas constexpr
        WindowingValues output = {};

        /*
        for (unsigned int i = 0; i < 6; ++i) {    // Already zero initialized so we don't need this
            output[i] = 0.0f;
        }*/

        for (unsigned int i = 6; i < 12; ++i) {
            output[i] = std::sin((M_PI / 12.0f) * (i - 5.5f)); // - 5.5f = - 6.0f + 0.5f
        }

        for (unsigned int i = 12; i < 18; ++i) {
            output[i] = 1.0f;
        }

        for (unsigned int i = 18; i < 36; ++i) {
            output[i] = std::sin((M_PI / 36.0f) * (i + 0.5f));
        }

        return output;
    }

    std::array<WindowingValues, 4> const windowingValuesPerBlock = {
        getWindowingValuesBlock0(),
        getWindowingValuesBlock1(),
        getWindowingValuesBlock2(),
        getWindowingValuesBlock3()
    };

}

#endif /* MP3_FRAME_DATA_SYNTHESISFILTERBANK_HPP */
