
#include <cassert>
#include "Helper.hpp"


namespace MP3::Helper {

    int getBitsAtIndex(std::vector<uint8_t> const &data, unsigned int &index, unsigned int size) {
        assert(size < (sizeof(int) * 8));

        //TODO: tester si on ne depasse pas la taille de data avec index

        int value = 0;

        for (; size > 0; --size, ++index) {
            value <<= 1;

            if ((index / 8) < data.size()) {
                value |= (data[index / 8] >> (7 - (index % 8))) & 0x1;
            }
        }
        
        return value;
    }
    
    std::vector<uint8_t> getDataFromStream(std::istream &inputStream, unsigned int sizeInBits) {//TODO: si sizeInBits n'est pas multiple de 8 ???
        std::vector<uint8_t> data;

        // Get size in bytes
        auto sizeInBytes = (sizeInBits / 8) + (((sizeInBits % 8) != 0) ? 1 : 0);

        // Reserve size
        data.reserve(sizeInBytes);//TODO: a voir car ne modifie pas la taille du vector
        data.resize(sizeInBytes);

        // TODO: verifier taille flux

        // Read data
        inputStream.read(reinterpret_cast<char *>(data.data()), sizeInBytes);

        return data;
    }

}