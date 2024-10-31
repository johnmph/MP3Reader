#ifndef MP3_HELPER_HPP
#define MP3_HELPER_HPP

#include <cstdint>
#include <vector>
#include <istream>
#include <cassert>


namespace MP3::Helper {

    std::vector<uint8_t> getDataFromStream(std::istream &inputStream, unsigned int sizeInBits);     //TODO: surement en octets plutot

    template <typename TValue, typename TData>
    TValue getBitsAtIndex(TData const &data, unsigned int &index, unsigned int size);
    
    template <typename TContainer, class TFunction>
    constexpr TContainer transformContainer(TContainer const &input, TFunction &&function);

    #include "Helper_s.hpp"

}

#endif /* MP3_HELPER_HPP */
