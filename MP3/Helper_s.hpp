#ifndef MP3_HELPER_S_HPP
#define MP3_HELPER_S_HPP


template <typename TValue>
TValue revertEndianness(TValue value) {
    TValue convertedValue = 0;

    // Browse all bytes of value
    for (unsigned int byteIndex = 0; byteIndex < sizeof(TValue); ++byteIndex) {
        convertedValue |= ((value >> (byteIndex * 8)) & 0xFF) << ((sizeof(TValue) - (byteIndex + 1)) * 8);
    }

    return convertedValue;
}

template <typename TValue, typename TData>
TValue getBitsAtIndex(TData const &data, unsigned int &index, unsigned int size) {
    assert(size <= (sizeof(TValue) * 8));

    //TODO: tester si on ne depasse pas la taille de data avec index : c'est fait mais voir si pas plutot un assert ?
    
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
constexpr TContainer transformContainer(TContainer container, TFunction &&function) {
    // Browse container
    for (std::size_t index = 0; index < container.size(); ++index) {
        // Apply transform function
        container[index] = function(container[index]);
    }

    return container;
}

template <typename TValue, TValue Polynomial, TValue InitialValue>
TValue calculateCRC(std::vector<uint8_t> const &data) {
    TValue crc = InitialValue;
   
    // Browse data
    for (std::size_t dataIndex = 0; dataIndex < data.size(); ++dataIndex) {
        // Get current value
        TValue currentValue = data[dataIndex] << 8;
       
        // Browse bits of currentValue
        for (std::size_t bitIndex = 0; bitIndex < 8; ++bitIndex) {
            // Check if need to apply polynomial
            bool applyPolynomial = ((crc ^ currentValue) & 0x8000) == 0x8000;
           
            // Shift left
            currentValue <<= 1;
            crc <<= 1;
           
            // Apply polynomial if needed
            if (applyPolynomial) {
                crc ^= Polynomial;
            }
        }
    }
   
    return crc;
}

#endif /* MP3_HELPER_S_HPP */
