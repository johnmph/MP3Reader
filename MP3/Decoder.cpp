
#include <array>
#include <algorithm>
#include "Decoder.hpp"
#include "Helper.hpp"
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

    Frame::Frame Decoder::getFrameAtIndex(std::istream &inputStream, unsigned int const frameIndex) {
        // Get frame header at frameIndex
        auto const frameHeader = tryToGetFrameHeaderAtIndex(inputStream, frameIndex);

        // If no frame header, error
        if (frameHeader.has_value() == false) {
            throw FrameNotFound(*this, frameIndex);//TODO: a voir
        }

        // Check CRC if needed
        if ((*frameHeader).isCRCProtected() == true) {
            // Get CRC stored and calculated
            auto crcStored = getCRCStored(inputStream, (*frameHeader));
            auto crcCalculated = getCRCCalculated(inputStream, (*frameHeader));

            // Check CRC
            if (crcStored != crcCalculated) {
                throw FrameCRCIncorrect(*this, crcStored, crcCalculated);//TODO: a voir
            }
        }
        
        // Create side informations
        Frame::SideInformation const frameSideInformation = getFrameSideInformation(inputStream, (*frameHeader));

        // Get frame data from bit reservoir
        auto const frameData = getFrameDataFromBitReservoir(inputStream, frameIndex, frameSideInformation);

        // Get frame ancillary data size in bits
        unsigned int const frameAncillaryDataSizeInBits = 0; //TODO: changer, attention dans le cas ou toutes les données d'une frame sont dans la frame précédente

        // Create frame
        return Frame::Frame((*frameHeader), frameSideInformation, frameAncillaryDataSizeInBits, frameData, _framesBlocksSubbandsOverlappingValues, _framesShiftedAndMatrixedSubbandsValues);
    }

    std::vector<uint8_t> Decoder::getFrameDataFromBitReservoir(std::istream &inputStream, unsigned int const frameIndex, Frame::SideInformation const &frameSideInformation) {
        // Get frame data
        std::vector<uint8_t> frameData((frameSideInformation.getMainDataSizeInBits() / 8) + (((frameSideInformation.getMainDataSizeInBits() % 8) != 0) ? 1 : 0));
        unsigned int bitReservoirCurrentFrameIndex = frameIndex + ((frameSideInformation.getMainDataBegin() == 0) ? 1 : 0);

        // Loop to find first frame to read
        std::optional<Frame::Header> bitReservoirCurrentFrameHeader;
        auto beginOffset = frameSideInformation.getMainDataBegin();

        for (;;) {
            // If first frame and we don't have all data, error
            if (bitReservoirCurrentFrameIndex == 0) {//TODO: voir si mettre ici ou dans getFrameHeaderAtIndex et si oui voir pour le type unsigned int ou signed car -1
                // TODO: thrower exception: plutot un code erreur car c'est le fichier qui est mal formé et pas une erreur de programmation !!!
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

        return frameData;
    }

    uint16_t Decoder::getCRCStored(std::istream &inputStream, Frame::Header const &frameHeader) {
        // Get stored CRC
        uint16_t storedCRC;
        inputStream.read(reinterpret_cast<char *>(&storedCRC), frameHeader.getCRCSize());

        // Convert in little endianness
        storedCRC = ((storedCRC << 8) & 0xFF00) | (storedCRC >> 8);

        // Avoid stream head change position
        inputStream.seekg(-static_cast<int>(frameHeader.getCRCSize()), std::ios_base::cur);

        return storedCRC;
    }

    uint16_t Decoder::getCRCCalculated(std::istream &inputStream, Frame::Header const &frameHeader) {
        // Data used in CRC is last 2 bytes of header and all side informations bytes
        std::vector<uint8_t> data(2 + frameHeader.getSideInformationSize());

        // Need to read 2 last bytes of header first
        inputStream.seekg(-2, std::ios_base::cur);
        inputStream.read(reinterpret_cast<char *>(data.data()), 2);

        // Pass CRC bytes
        inputStream.seekg(frameHeader.getCRCSize(), std::ios_base::cur);

        // Need to read all side informations bytes
        inputStream.read(reinterpret_cast<char *>(&data.data()[2]), frameHeader.getSideInformationSize());

        // Calculate CRC
        auto calculatedCRC = Helper::calculateCRC<uint16_t, 0x8005, 0xFFFF>(data);

        // Avoid stream head change position
        inputStream.seekg(-static_cast<int>(frameHeader.getSideInformationSize()), std::ios_base::cur);//TODO: voir si les autres methodes font la meme chose quand elles bougent la tete de lecture pour la remettre a la fin de la methode pour eviter les effets de bords sauf celles dont c'est le but (et si c'est le but les renommer en move)

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

    Frame::SideInformation Decoder::getFrameSideInformation(std::istream &inputStream, Frame::Header const &frameHeader) const {
        // Get SideInformation
        return Frame::SideInformation(frameHeader, Helper::getDataFromStream(inputStream, frameHeader.getSideInformationSize() * 8));//TODO : voir si garder getDataFromStream en bits!
    }

}
