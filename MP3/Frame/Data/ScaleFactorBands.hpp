#ifndef MP3_FRAME_DATA_SCALEFACTORBANDS_HPP
#define MP3_FRAME_DATA_SCALEFACTORBANDS_HPP

#include <array>


namespace MP3::Frame::Data {//TODO: voir pour les dernieres valeurs pour couvrir toutes les frequency lines si on laisse ainsi ou si on change le code qui utilise ces tables !!!

    using FrequencyLinesPerScaleFactorBandShortBlock = std::array<unsigned int, 13>;
    using FrequencyLinesPerScaleFactorBandLongBlock = std::array<unsigned int, 22>;

    constexpr FrequencyLinesPerScaleFactorBandLongBlock frequencyLinesPerScaleFactorBandLongBlock32000 = {
        3,
        7,
        11,
        15,
        19,
        23,
        29,
        35,
        43,
        53,
        65,
        81,
        101,
        125,
        155,
        193,
        239,
        295,
        363,
        447,
        549,
        575
    };

    constexpr FrequencyLinesPerScaleFactorBandLongBlock frequencyLinesPerScaleFactorBandLongBlock44100 = {
        3,
        7,
        11,
        15,
        19,
        23,
        29,
        35,
        43,
        51,
        61,
        73,
        89,
        109,
        133,
        161,
        195,
        237,
        287,
        341,
        417,
        575
    };

    constexpr FrequencyLinesPerScaleFactorBandLongBlock frequencyLinesPerScaleFactorBandLongBlock48000 = {
        3,
        7,
        11,
        15,
        19,
        23,
        29,
        35,
        41,
        49,
        59,
        71,
        87,
        105,
        127,
        155,
        189,
        229,
        275,
        329,
        383,
        575
    };

    constexpr std::array<FrequencyLinesPerScaleFactorBandLongBlock, 3> frequencyLinesPerScaleFactorBandLongBlock = {
        frequencyLinesPerScaleFactorBandLongBlock44100,
        frequencyLinesPerScaleFactorBandLongBlock48000,
        frequencyLinesPerScaleFactorBandLongBlock32000
    };

    constexpr FrequencyLinesPerScaleFactorBandShortBlock frequencyLinesPerScaleFactorBandShortBlock32000 = {
        3,
        7,
        11,
        15,
        21,
        29,
        41,
        57,
        77,
        103,
        137,
        179,
        191
    };

    constexpr FrequencyLinesPerScaleFactorBandShortBlock frequencyLinesPerScaleFactorBandShortBlock44100 = {
        3,
        7,
        11,
        15,
        21,
        29,
        39,
        51,
        65,
        83,
        105,
        135,
        191
    };

    constexpr FrequencyLinesPerScaleFactorBandShortBlock frequencyLinesPerScaleFactorBandShortBlock48000 = {
        3,
        7,
        11,
        15,
        21,
        27,
        37,
        49,
        63,
        79,
        99,
        125,
        191
    };

    constexpr std::array<FrequencyLinesPerScaleFactorBandShortBlock, 3> frequencyLinesPerScaleFactorBandShortBlock = {
        frequencyLinesPerScaleFactorBandShortBlock44100,
        frequencyLinesPerScaleFactorBandShortBlock48000,
        frequencyLinesPerScaleFactorBandShortBlock32000
    };

}

#endif /* MP3_FRAME_DATA_SCALEFACTORBANDS_HPP */
