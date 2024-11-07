
#include "Helper.hpp"


namespace MP3::Helper {

    std::vector<uint8_t> getDataFromStream(std::istream &inputStream, unsigned int size) {
        // Create data
        std::vector<uint8_t> data(size, 0);

        // Read data
        inputStream.read(reinterpret_cast<char *>(data.data()), size);

        // Verify that we read all data
        if (inputStream.gcount() != size) {
            throw std::exception();//TODO: changer
        }
        
        return data;
    }
    
}