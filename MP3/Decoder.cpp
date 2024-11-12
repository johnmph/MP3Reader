
#include <array>
#include <algorithm>
#include <utility>
#include "Decoder.hpp"
#include "Frame/Data/Header.hpp"

//TODO: voir si avoir une structure pour toutes les erreurs rencontrées pour les avoir dans des flags qu'on puisse lire et meme peut etre avoir une structure avec des flags masks pour qu'on puisse definir si telle erreur est critique et doit arreter la lecture ou non, a voir si ca ou simple throw ou simple code erreur a retourner
namespace MP3 {

    Decoder::Decoder(uint8_t const versionMask, uint8_t const versionValue) : _versionMask(versionMask), _versionValue(versionValue), _framesBlocksSubbandsOverlappingValues({}), _framesShiftedAndMatrixedSubbandsValues({}) {//TODO: voir pour empty initializers, si pas bon, mettre std fill dans le constructor
        // TODO: throw ici si la version n'est pas supportée par le decoder
    }

    bool Decoder::isValidFormat(std::istream &inputStream, unsigned int const numberOfFramesForValidFormat) {
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

    unsigned int Decoder::getNumberOfFrames(std::istream &inputStream) {
        unsigned int frameCount = 0;

        browseFramesHeader(inputStream, [&inputStream, &frameCount](Frame::Header const &frameHeader) {
            // Increment frameCount
            ++frameCount;

            // Continue
            return true;
        });

        return frameCount;
    }

    std::unordered_set<unsigned int> Decoder::getBitrates(std::istream &inputStream) {
        std::unordered_set<unsigned int> bitrates;

        browseFramesHeader(inputStream, [&inputStream, &bitrates](Frame::Header const &frameHeader) {
            // Add current bitrate
            bitrates.insert(frameHeader.getBitrate());

            // Continue
            return true;
        });

        return bitrates;
    }

    std::unordered_set<unsigned int> Decoder::getSamplingRates(std::istream &inputStream) {
        std::unordered_set<unsigned int> samplingRates;

        browseFramesHeader(inputStream, [&inputStream, &samplingRates](Frame::Header const &frameHeader) {
            // Add current sampling rate
            samplingRates.insert(Frame::Data::samplingRates[frameHeader.getSamplingRateIndex()]);

            // Continue
            return true;
        });

        return samplingRates;
    }

    bool Decoder::getFrameDataFromBitReservoir(std::istream &inputStream, unsigned int const frameIndex, Frame::SideInformation const &frameSideInformation, std::vector<uint8_t> &frameData) {
        bool result = true;

        // Calculate current bit reservoir frame index
        unsigned int bitReservoirCurrentFrameIndex = frameIndex + ((frameSideInformation.getMainDataBegin() == 0) ? 1 : 0);

        // Loop to find first frame to read
        std::optional<Frame::Header> bitReservoirCurrentFrameHeader;
        auto beginOffset = frameSideInformation.getMainDataBegin();

        for (;;) {
            // If first frame and we don't have all data, error
            if (bitReservoirCurrentFrameIndex == 0) {
                result = false;
                return {};//TODO: a la place de quitter avec une erreur sans rien lire, lire la partie qu'on sait et laisser des 0 au debut sur tout ce qu'on n'a pas su lire mais quand meme notifier l'erreur pour qu'on puisse thrower dans la methode appelante tout en ayant les données dispo pour l'exception
            }

            // Go to previous frame
            --bitReservoirCurrentFrameIndex;
            bitReservoirCurrentFrameHeader = tryToGetFrameHeaderAtIndex(inputStream, bitReservoirCurrentFrameIndex);

            // Check if we have enough data
            if ((*bitReservoirCurrentFrameHeader).getDataSize() >= beginOffset) {
                // Exit loop
                break;
            }

            // Substract bitReservoir current frame data size from offset
            beginOffset -= (*bitReservoirCurrentFrameHeader).getDataSize();
        }

        // Loop to read data
        for (unsigned int bytesRead = 0; bytesRead < frameData.size();) {
            // Set position of stream to data of bitReservoir current frame
            inputStream.seekg((*bitReservoirCurrentFrameHeader).getCRCSize() + (*bitReservoirCurrentFrameHeader).getSideInformationSize() + (((bitReservoirCurrentFrameIndex < frameIndex) && (bytesRead == 0)) ? ((*bitReservoirCurrentFrameHeader).getDataSize() - beginOffset) : 0), std::ios_base::cur);

            // Calculate available bytes
            unsigned int const limitToRead = ((bitReservoirCurrentFrameIndex < frameIndex) && (bytesRead == 0)) ? beginOffset : (*bitReservoirCurrentFrameHeader).getDataSize();
            unsigned int const bytesAvailable = ((frameData.size() - bytesRead) > limitToRead) ? limitToRead : (frameData.size() - bytesRead);

            // Read available bytes
            inputStream.read(reinterpret_cast<char *>(&frameData.data()[bytesRead]), bytesAvailable);
            bytesRead += bytesAvailable;

            // Go to next frame
            ++bitReservoirCurrentFrameIndex;
            bitReservoirCurrentFrameHeader = tryToGetFrameHeaderAtIndex(inputStream, bitReservoirCurrentFrameIndex);
        }

        return result;
    }

    uint16_t Decoder::getCRCIfExist(std::istream &inputStream, Frame::Header const &frameHeader) {
        // If no CRC protected, exit
        if (frameHeader.isCRCProtected() == false) {
            return 0;
        }

        // Get stored CRC
        uint16_t storedCRC;
        inputStream.read(reinterpret_cast<char *>(&storedCRC), frameHeader.getCRCSize());

        // Convert in little endianness
        storedCRC = ((storedCRC << 8) & 0xFF00) | (storedCRC >> 8);//TODO: mettre dans une fonction helper invertEndianness (template sur le type)

        // Return it
        return storedCRC;
    }

    uint16_t Decoder::calculateCRC(std::array<uint8_t, 4> const &headerData, std::vector<uint8_t> const &sideInformationData) {
        // Data used in CRC is last 2 bytes of header and all side informations bytes
        std::vector<uint8_t> data(2 + sideInformationData.size());

        // Copy 2 last bytes of header first
        std::copy(std::next(std::cbegin(headerData), 2), std::cend(headerData), std::begin(data));

        // Copy all side informations bytes
        std::copy(std::cbegin(sideInformationData), std::cend(sideInformationData), std::next(std::begin(data), 2));

        // Calculate CRC
        auto calculatedCRC = Helper::calculateCRC<uint16_t, 0x8005, 0xFFFF>(data);//TODO: mettre les valeurs dans des constantes

        // Return it
        return calculatedCRC;
    }

    std::optional<Frame::Header> Decoder::tryToGetFrameHeaderAtIndex(std::istream &inputStream, unsigned int const frameIndex) {
        inputStream.clear();//TODO: a voir

        // Create headerData
        std::optional<std::array<uint8_t, Frame::Header::headerSize>> headerData;

        // Need to search it if not already in array
        if (_frameEntries.size() <= frameIndex) {
            for (unsigned int currentFrameIndex = _frameEntries.size(); currentFrameIndex <= frameIndex; ++currentFrameIndex) {
                // Set stream cursor to beginning of current frame
                inputStream.seekg((currentFrameIndex > 0) ? (_frameEntries[currentFrameIndex - 1].positionInBytes + _frameEntries[currentFrameIndex - 1].sizeInBytes) : 0);

                // Try to synchronize to next frame
                headerData = tryToReadNextFrameHeaderData(inputStream);

                if (headerData.has_value() == false) {
                    return {};
                }

                // Create header
                Frame::Header header((*headerData), _versionMask, _versionValue);

                // Save to array
                _frameEntries.push_back({ static_cast<unsigned int>(inputStream.tellg()) - static_cast<unsigned int>((*headerData).size()), header.getFrameLength() });
            }
        }

        // Set stream cursor to beginning of frame
        inputStream.seekg(_frameEntries[frameIndex].positionInBytes);

        // Read frame header
        inputStream.read(reinterpret_cast<char *>((*headerData).data()), (*headerData).size());

        // Create frame header
        return Frame::Header((*headerData), _versionMask, _versionValue);
    }

    std::optional<std::array<uint8_t, Frame::Header::headerSize>> Decoder::tryToReadNextFrameHeaderData(std::istream &inputStream) const {
        // Create headerData
        std::array<uint8_t, Frame::Header::headerSize> headerData;

        // Read 4 first bytes if possible
        inputStream.read(reinterpret_cast<char *>(headerData.data()), headerData.size());

        // Browse input stream
        while (inputStream.good()) {
            // Check if header found
            if (Frame::Header::isValidHeader(headerData, _versionMask, _versionValue) == true) {
                // Found
                return headerData;
            }

            // Shift left the array and get next byte
            std::rotate(headerData.begin(), headerData.begin() + 1, headerData.end());
            headerData[3] = inputStream.get();
        }

        // Not found
        return {};
    }

}
