#ifndef MP3_HELPER_S_HPP
#define MP3_HELPER_S_HPP


template <typename TValue, typename TData>
TValue getBitsAtIndex(TData const &data, unsigned int &index, unsigned int size) {
    assert(size <= (sizeof(TValue) * 8));

    //TODO: tester si on ne depasse pas la taille de data avec index
    
    TValue value = 0;

    for (; size > 0; --size, ++index) {
        value <<= 1;

        if ((index / 8) < data.size()) {
            value |= (data[index / 8] >> (7 - (index % 8))) & 0x1;
        }
    }
    
    return value;
}

template <typename TContainer, class TFunction>
constexpr TContainer transformContainer(TContainer const &input, TFunction &&function) {    //TODO: ATTENTION: si vector, taille pas initialisée
    TContainer output = {};

    for (std::size_t index = 0; index < input.size(); ++index) {
        output[index] = function(input[index]);
    }

    return output;
}


#endif /* MP3_HELPER_S_HPP */
