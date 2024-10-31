
#include "Helper.hpp"


namespace MP3::Helper {

    std::vector<uint8_t> getDataFromStream(std::istream &inputStream, unsigned int sizeInBits) {//TODO: si sizeInBits n'est pas multiple de 8 ???
        // Get size in bytes
        auto sizeInBytes = (sizeInBits / 8) + (((sizeInBits % 8) != 0) ? 1 : 0);

        // Create data
        std::vector<uint8_t> data(sizeInBytes);

        // TODO: verifier taille flux

        // Read data
        inputStream.read(reinterpret_cast<char *>(data.data()), sizeInBytes);

        return data;
    }

}