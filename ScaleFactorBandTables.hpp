#ifndef SCALEFACTORBANDTABLES_HPP
#define SCALEFACTORBANDTABLES_HPP

#include <array>

// TODO: namespace

using ScaleFactorBandsShortBlockTable = std::array<int, 12>;
using ScaleFactorBandsLongBlockTable = std::array<int, 21>;

ScaleFactorBandsLongBlockTable const scaleFactorBandsLongBlockTable32000 = {
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
    549
};

ScaleFactorBandsLongBlockTable const scaleFactorBandsLongBlockTable44100 = {
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
    417
};

ScaleFactorBandsLongBlockTable const scaleFactorBandsLongBlockTable48000 = {
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
    383
};

std::array<ScaleFactorBandsLongBlockTable, 3> const scaleFactorBandsLongBlockTable = {
    scaleFactorBandsLongBlockTable44100,
    scaleFactorBandsLongBlockTable48000,
    scaleFactorBandsLongBlockTable32000
};

ScaleFactorBandsShortBlockTable const scaleFactorBandsShortBlockTable32000 = {
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
    179
};

ScaleFactorBandsShortBlockTable const scaleFactorBandsShortBlockTable44100 = {
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
    135
};

ScaleFactorBandsShortBlockTable const scaleFactorBandsShortBlockTable48000 = {
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
    125
};

std::array<ScaleFactorBandsShortBlockTable, 3> const scaleFactorBandsShortBlockTable = {
    scaleFactorBandsShortBlockTable44100,
    scaleFactorBandsShortBlockTable48000,
    scaleFactorBandsShortBlockTable32000
};

#endif /* SCALEFACTORBANDTABLES_HPP */
