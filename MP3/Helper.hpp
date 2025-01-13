#ifndef MP3_HELPER_HPP
#define MP3_HELPER_HPP

#include <cstdint>
#include <vector>
#include <istream>
#include <cassert>


namespace MP3::Helper {

    std::vector<uint8_t> getDataFromStream(std::istream &inputStream, unsigned int size);

    template <typename TValue>
    TValue revertEndianness(TValue value);
    
    template <typename TValue, typename TData>
    TValue getBitsAtIndex(TData const &data, unsigned int &index, unsigned int size);
    
    template <typename TContainer, class TFunction>
    constexpr TContainer transformContainer(TContainer container, TFunction &&function);

    template <typename TValue, TValue Polynomial, TValue InitialValue>
    TValue calculateCRC(std::vector<uint8_t> const &data);


    #include "Helper_s.hpp"

}

#endif /* MP3_HELPER_HPP */
