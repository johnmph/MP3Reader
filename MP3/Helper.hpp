#ifndef MP3_HELPER_HPP
#define MP3_HELPER_HPP

#include <cstdint>
#include <vector>
#include <istream>


namespace MP3::Helper {

    int getBitsAtIndex(std::vector<uint8_t> const &data, unsigned int &index, unsigned int size);
    std::vector<uint8_t> getDataFromStream(std::istream &inputStream, unsigned int sizeInBits);     //TODO: surement en octets plutot

    template <typename TContainer, class TFunction>
    constexpr TContainer transformContainer(TContainer const &input, TFunction &&function);

    #include "Helper_s.hpp"

}

#endif /* MP3_HELPER_HPP */
