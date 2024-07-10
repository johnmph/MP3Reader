
#include <array>
#include <algorithm>
#include "Decoder.hpp"
#include "Helper.hpp"
#include "Frame/Data/Header.hpp"


namespace MP3 {

    Decoder::Decoder() : _framesBlocksSubbandsOverlappingValues({}), _framesShiftedAndMatrixedSubbandsValues({}) {//TODO: voir pour empty initializers, si pas bon, mettre std fill dans le constructor
    }

    bool Decoder::isValidFormat(std::istream &inputStream, unsigned int const numberOfFramesForValidFormat) const {
        bool isValid = false;


        unsigned int oldPosition = 0;
        unsigned int frameCount = 0;

        browseFramesHeader(inputStream, [&inputStream, &isValid, &oldPosition, &frameCount, &numberOfFramesForValidFormat](Frame::Header const &frameHeader) {
            // Get current position in stream
            unsigned int const currentPosition = inputStream.tellg();

            // Restart if next frame not just after current frame
            if (static_cast<unsigned int>(currentPosition) > (oldPosition + Frame::Header::headerSize)) {
                // Reset frame count (-1 to have 0 after incrementation)
                frameCount = -1;
            }

            // Increment frame count
            ++frameCount;

            // If we have enough frames for valid format
            if (frameCount >= numberOfFramesForValidFormat) {
                isValid = true;

                // Exit
                return false;
            }

            // Update old position
            oldPosition = currentPosition + (frameHeader.getFrameLength() - Frame::Header::headerSize);

            // Continue
            return true;
        });

        return isValid;
    }

    unsigned int Decoder::getNumberOfFrames(std::istream &inputStream) const {
        unsigned int frameCount = 0;

        browseFramesHeader(inputStream, [&inputStream, &frameCount](Frame::Header const &frameHeader) {
            // Increment frameCount
            ++frameCount;

            // Continue
            return true;
        });

        return frameCount;
    }

    std::unordered_set<unsigned int> Decoder::getBitrates(std::istream &inputStream) const {
        std::unordered_set<unsigned int> bitrates;

        browseFramesHeader(inputStream, [&inputStream, &bitrates](Frame::Header const &frameHeader) {
            // Add current bitrate
            bitrates.insert(frameHeader.getBitrate());

            // Continue
            return true;
        });

        return bitrates;
    }

    std::unordered_set<unsigned int> Decoder::getSamplingRates(std::istream &inputStream) const {
        std::unordered_set<unsigned int> samplingRates;

        browseFramesHeader(inputStream, [&inputStream, &samplingRates](Frame::Header const &frameHeader) {
            // Add current sampling rate
            samplingRates.insert(Frame::Data::samplingRates[frameHeader.getSamplingRateIndex()]);

            // Continue
            return true;
        });

        return samplingRates;
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
            if (bitReservoirCurrentFrameHeader.getDataSize() >= beginOffset) {
                // Read all data
                for (unsigned int bytesRead = 0; bytesRead < frameData.size();) {
                    // Set position of stream to data of bitReservoir current frame
                    inputStream.seekg(bitReservoirCurrentFrameHeader.getCRCSize() + bitReservoirCurrentFrameHeader.getSideInformationSize() + (((bitReservoirCurrentFrameIndex < frameIndex) && (bytesRead == 0)) ? (bitReservoirCurrentFrameHeader.getDataSize() - beginOffset) : 0), std::ios_base::cur);

                    // Calculate available bytes
                    unsigned int const limitToRead = ((bitReservoirCurrentFrameIndex < frameIndex) && (bytesRead == 0)) ? beginOffset : bitReservoirCurrentFrameHeader.getDataSize();
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
        return Frame::Frame(frameHeader, frameSideInformation, frameAncillaryDataSizeInBits, frameData, _framesBlocksSubbandsOverlappingValues, _framesShiftedAndMatrixedSubbandsValues);
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
            throw std::invalid_argument("headerBytes is incorrect");  // TODO: changer par une classe custom
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
