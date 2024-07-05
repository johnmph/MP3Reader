
#include <array>
#include <algorithm>
#include "Decoder.hpp"
#include "Helper.hpp"


namespace MP3 {

    /*Decoder::Decoder() {
    }*/

    bool Decoder::isValidFormat(std::istream &inputStream, unsigned int const numberOfFramesForValidFormat) const {
        // Reset stream
        inputStream.clear();
        inputStream.seekg(0);

        // Try to find at least numberOfFramesForValidFormat frames
        std::array<uint8_t, Frame::Header::headerSize> headerData;
        bool isValid = true;

        for (unsigned int frameIndex = 0; frameIndex < numberOfFramesForValidFormat; ++frameIndex) {
            // Get current position in stream
            unsigned int const currentPosition = inputStream.tellg();

            // Exit if we can't synchronize to next frame
            if (synchronizeToNextFrame(inputStream, headerData, 0xFE, 0xFA) == false) {
                // Format is not valid
                isValid = false;
                break;
            }

            // Restart if next frame not just after current frame
            if (static_cast<unsigned int>(inputStream.tellg()) > (currentPosition + headerData.size())) {
                frameIndex = 0;
            }

            // Create header
            Frame::Header header(headerData, 0xFE, 0xFA);

            // Pass frame size bytes in stream (minus header size)
            inputStream.seekg(header.getFrameLength() - headerData.size(), std::ios_base::cur);
        }

        // Reset stream
        inputStream.clear();
        inputStream.seekg(0);   //TODO: a voir

        // Return isValid
        return isValid;
    }

    unsigned int Decoder::getNumberOfFrames(std::istream &inputStream) const {//TODO: factoriser ce code avec celui du dessus
        // Reset stream
        inputStream.clear();
        inputStream.seekg(0);

        // Try to find at least numberOfFramesForValidFormat frames
        std::array<uint8_t, Frame::Header::headerSize> headerData;

        // While inputStream is good
        unsigned int frameCount = 0;

        while (inputStream) {
            // Get current position in stream
            unsigned int const currentPosition = inputStream.tellg();

            // Exit if we can't synchronize to next frame
            if (synchronizeToNextFrame(inputStream, headerData, 0xFE, 0xFA) == false) {
                // Can't find another frame
                break;
            }

            // Restart if next frame not just after current frame
            if (static_cast<unsigned int>(inputStream.tellg()) > (currentPosition + headerData.size())) {
                frameCount = 0;
            }

            // Create header
            Frame::Header header(headerData, 0xFE, 0xFA);

            // Pass frame size bytes in stream (minus header size)
            inputStream.seekg(header.getFrameLength() - headerData.size(), std::ios_base::cur);

            // Increment frameCount
            ++frameCount;
        }

        // Reset stream
        inputStream.clear();
        inputStream.seekg(0);   //TODO: a voir

        // Return frameCount
        return frameCount;
    }

    Frame::Frame Decoder::getFrameAtIndex(std::istream &inputStream, unsigned int const frameIndex) {
        // Get frame header at frameIndex
        auto const frameHeader = getFrameHeaderAtIndex(inputStream, frameIndex);

        // TODO: si CRC, le lire
        if (frameHeader.isCRCProtected() == true) {
            inputStream.seekg(frameHeader.getCRCSize(), std::ios_base::cur);
        }
        
        // Create side informations
        Frame::SideInformation const frameSideInformation(frameHeader, getSideInformationData(inputStream, frameHeader));

        // Get frame data
    int p = frameSideInformation.getMainDataBegin();//TODO: a retirer et les comms en bas
    if (/*p > 591*/frameIndex == 11) {
        int r = 1;
    }
        std::vector<uint8_t> frameData((frameSideInformation.getMainDataSizeInBits() / 8) + (((frameSideInformation.getMainDataSizeInBits() % 8) != 0) ? 1 : 0));
        unsigned int bitReservoirCurrentFrameIndex = frameIndex + ((frameSideInformation.getMainDataBegin() == 0) ? 1 : 0);

        for (auto beginOffset = frameSideInformation.getMainDataBegin();;) {
            // If first frame and we don't have all data, error
            if (bitReservoirCurrentFrameIndex == 0) {//TODO: voir si mettre ici ou dans getFrameHeaderAtIndex et si oui voir pour le type unsigned int ou signed car -1
                // TODO: thrower exception
            }

            // Go to previous frame
            --bitReservoirCurrentFrameIndex;
            auto bitReservoirCurrentFrameHeader = getFrameHeaderAtIndex(inputStream, bitReservoirCurrentFrameIndex);

            // Check if we have enough data
        int ee = bitReservoirCurrentFrameHeader.getDataSize();
            if (bitReservoirCurrentFrameHeader.getDataSize() >= beginOffset) {
                // Read all data
                for (unsigned int bytesRead = 0; bytesRead < frameData.size();) {
                    // Set position of stream to data of bitReservoir current frame
                int r = inputStream.tellg();
                    inputStream.seekg(bitReservoirCurrentFrameHeader.getCRCSize() + bitReservoirCurrentFrameHeader.getSideInformationSize() + ((bytesRead == 0) ? (bitReservoirCurrentFrameHeader.getDataSize() - beginOffset) : 0), std::ios_base::cur);
                int x = bitReservoirCurrentFrameHeader.getCRCSize() + bitReservoirCurrentFrameHeader.getSideInformationSize() + ((bytesRead == 0) ? (bitReservoirCurrentFrameHeader.getDataSize() - beginOffset) : 0);
                int r2 = inputStream.tellg();

                    // Calculate available bytes
                    unsigned int const limitToRead = (bytesRead == 0) ? beginOffset : bitReservoirCurrentFrameHeader.getDataSize();
                    unsigned int const bytesAvailable = ((frameData.size() - bytesRead) > limitToRead) ? limitToRead : (frameData.size() - bytesRead);

                    // Read available bytes
                    inputStream.read(reinterpret_cast<char *>(&frameData.data()[bytesRead]), bytesAvailable);
                    bytesRead += bytesAvailable;

                    // Go to next frame
                    ++bitReservoirCurrentFrameIndex;
                    bitReservoirCurrentFrameHeader = getFrameHeaderAtIndex(inputStream, bitReservoirCurrentFrameIndex);
                }

                // Exit loop
                break;
            } else {
                // Substract bitReservoir current frame data size from offset
                beginOffset -= bitReservoirCurrentFrameHeader.getDataSize();
            }
        }

        unsigned int const frameAncillaryDataSizeInBits = 0; //TODO: changer, attention dans le cas ou toutes les données d'une frame sont dans la frame précédente

        // Create frame
        return Frame::Frame(frameHeader, frameSideInformation, frameAncillaryDataSizeInBits, frameData);
    }

    bool Decoder::synchronizeToNextFrame(std::istream &inputStream, std::array<uint8_t, Frame::Header::headerSize> &headerData, uint8_t const versionMask, uint8_t const versionValue) const {
        // Read 4 first bytes if possible
        inputStream.read(reinterpret_cast<char *>(headerData.data()), headerData.size());

        // Browse input stream
        while (inputStream.good()) {
            // Check if header found
            if (Frame::Header::isValidHeader(headerData, versionMask, versionValue) == true) {
                // Found
                return true;
            }

            // Shift left the array and get next byte
            std::rotate(headerData.begin(), headerData.begin() + 1, headerData.end());
            headerData[3] = inputStream.get();
        }

        // Not found
        return false;
    }

    Frame::Header Decoder::getFrameHeaderAtIndex(std::istream &inputStream, unsigned int const frameIndex) {
        // Move to frame at frameIndex
        if (moveToFrameAtIndex(inputStream, frameIndex) == false) {
            // TODO: exception ?
        }

        // Create header
        std::array<uint8_t, Frame::Header::headerSize> headerData;

        if (synchronizeToNextFrame(inputStream, headerData, 0xFE, 0xFA) == false) {
            // TODO: exception ?
        }

        return Frame::Header(headerData, 0xFE, 0xFA);
    }

    bool Decoder::moveToFrameAtIndex(std::istream &inputStream, unsigned int const frameIndex) {
        inputStream.clear();//TODO: a voir

        // Need to search it if not already in array
        if (_frameEntries.size() <= frameIndex) {
            // Need to find it
            std::array<uint8_t, Frame::Header::headerSize> headerData;

            for (unsigned int currentFrameIndex = _frameEntries.size(); currentFrameIndex <= frameIndex; ++currentFrameIndex) {
                // Set stream cursor to beginning of current frame
                inputStream.seekg((currentFrameIndex > 0) ? (_frameEntries[currentFrameIndex - 1].positionInBytes + _frameEntries[currentFrameIndex - 1].sizeInBytes) : 0);

                // Synchronize to next frame
                if (synchronizeToNextFrame(inputStream, headerData, 0xFE, 0xFA) == false) {
                    // Not found, exit
                    return false;
                }

                // Create header
                Frame::Header header(headerData, 0xFE, 0xFA);

                // Save to array
                _frameEntries.push_back({ static_cast<unsigned int>(inputStream.tellg()) - static_cast<unsigned int>(headerData.size()), header.getFrameLength() });
            }
        }

        // Set stream cursor to beginning of frame
        inputStream.seekg(_frameEntries[frameIndex].positionInBytes);

        // Ok
        return true;
    }

    std::vector<uint8_t> Decoder::getSideInformationData(std::istream &inputStream, Frame::Header const &header) const {
        // Get data from stream
        return Helper::getDataFromStream(inputStream, header.getSideInformationSize() * 8);//TODO : voir si garder getDataFromStream en bits!
    }

}
