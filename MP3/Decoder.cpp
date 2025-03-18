
#include <array>
#include <algorithm>
#include <utility>
#include "Decoder.hpp"
#include "Frame/Data/Header.hpp"


namespace MP3 {

    bool Decoder::isValidFormat(std::istream &inputStream, unsigned int const numberOfFramesForValidFormat) {
        // Format is valid if we found a first valid frame
        return findFirstValidFrame(inputStream, numberOfFramesForValidFormat).has_value();
    }

    Decoder::Decoder(std::unique_ptr<std::istream> inputStream, unsigned int const numberOfFramesForValidFormat) : _inputStream(std::move(inputStream)), _framesBlocksSubbandsOverlappingValues({}), _framesShiftedAndMatrixedSubbandsValues({}) {
        // Get first valid frame
        auto firstValidFrame = findFirstValidFrame((*_inputStream), numberOfFramesForValidFormat);

        // If first valid frame not found
        if (firstValidFrame.has_value() == false) {
            // Format is invalid
            throw Error::BadMP3Format(*this, *_inputStream);//TODO: attention la reference this sera invalide !!!
        }

        // Set first valid frame
        _frameEntries.push_back((*firstValidFrame));
    }

    unsigned int Decoder::getNumberOfFrames() {
        unsigned int frameCount = 0;

        browseFramesHeader([&frameCount](Frame::Header const &frameHeader) {
            // Increment frameCount
            ++frameCount;

            // Continue
            return true;
        });

        return frameCount;
    }

    std::unordered_set<unsigned int> const &Decoder::getBitrates() {
        // Lazy creation
        if (_bitrates.empty() == true) {
            browseFramesHeader([&bitrates = _bitrates](Frame::Header const &frameHeader) {
                // Add current bitrate
                bitrates.insert(frameHeader.getBitrate());

                // Continue
                return true;
            });
        }

        return _bitrates;
    }

    std::unordered_set<unsigned int> const &Decoder::getSamplingRates() {
        // Lazy creation
        if (_samplingRates.empty() == true) {
            browseFramesHeader([&samplingRates = _samplingRates](Frame::Header const &frameHeader) {
                // Add current sampling rate
                samplingRates.insert(frameHeader.getSamplingRate());

                // Continue
                return true;
            });
        }

        return _samplingRates;
    }

    std::optional<Decoder::FrameEntry> Decoder::findFirstValidFrame(std::istream &inputStream, unsigned int const numberOfFramesForValidFormat) {
        // Set stream cursor to beginning
        inputStream.clear();
        inputStream.seekg(0);

        std::optional<Decoder::FrameEntry> frameEntry;
        
        // Create headerData
        std::optional<std::array<uint8_t, Frame::Header::headerSize>> headerData;

        unsigned int lastPosition = 0;
        unsigned int currentPosition = 0;
        for (unsigned int currentFrameIndex = 0; currentFrameIndex < numberOfFramesForValidFormat; ++currentFrameIndex) {
            // Try to synchronize to next frame
            headerData = tryToReadNextFrameHeaderData(inputStream);

            // If frame not found
            if (headerData.has_value() == false) {
                // Reset frameEntry and exit loop
                frameEntry = std::nullopt;
                break;
            }

            // Set stream cursor at header start
            inputStream.seekg(-(*headerData).size(), std::ios::cur);
            
            // Check if it is not a consecutive frame
            if ((currentFrameIndex > 0) && (inputStream.tellg() != lastPosition)) {
                // Restart by passing to next byte (-1 to currentFrameIndex to have it = 0 when continue is applied)
                currentFrameIndex = -1;
                inputStream.seekg(currentPosition + 1);

                continue;
            }

            // Get current position in stream
            currentPosition = inputStream.tellg();

            // Create header
            Frame::Header header((*headerData), versionMask, versionValue);

            // Get frame length (or 1 in case of incorrect header)
            auto const frameLength = std::max<unsigned int>(header.getFrameLength(), 1);//TODO: une frame length = 0 ne peut pas arriver ainsi car j'ai le test dans le isValid pour etre sur que bitrate et samplingrate sont ok, mais dans le cas ou je retire ces tests et que je les mets dans decode ! reflechir car si je les mets la, il faut trouver un moyen pour pouvoir corriger avec toutes les données nécessaires qu'on a pas dans header !

            // Save first possible valid frame header to frameEntry
            if (currentFrameIndex == 0) {
                frameEntry = { currentPosition, frameLength };
            }

            // Set stream cursor to beginning of next frame or to next byte if frame has no length (in case of incorrect header)
            inputStream.seekg(frameLength, std::ios::cur);

            // Get last position
            lastPosition = inputStream.tellg();
        }

        // Valid
        return frameEntry;
    }

    std::optional<std::array<uint8_t, Frame::Header::headerSize>> Decoder::tryToReadNextFrameHeaderData(std::istream &inputStream) {
        // Create headerData
        std::array<uint8_t, Frame::Header::headerSize> headerData;

        // Read 4 first bytes if possible
        inputStream.read(reinterpret_cast<char *>(headerData.data()), headerData.size());

        // Browse input stream
        while (inputStream.good() == true) {
            // Check if header found
            if (Frame::Header::isValidHeader(headerData, versionMask, versionValue) == true) {
                // Found
                return headerData;
            }

            // Shift left the array and get next byte
            std::rotate(std::begin(headerData), std::next(std::begin(headerData), 1), std::end(headerData));
            headerData[3] = inputStream.get();
        }

        // Not found
        return {};
    }

    bool Decoder::getFrameDataFromBitReservoir(unsigned int const frameIndex, Frame::SideInformation const &frameSideInformation, std::vector<uint8_t> &frameData) {
        bool result = true;

        // Calculate current bit reservoir frame index
        unsigned int bitReservoirCurrentFrameIndex = frameIndex + ((frameSideInformation.getMainDataBegin() == 0) ? 1 : 0);

        // Loop to find first frame to read
        std::optional<Frame::Header> bitReservoirCurrentFrameHeader;
        auto beginOffset = frameSideInformation.getMainDataBegin();

        unsigned int bytesReadStart = 0;

        for (;;) {
            // If first frame and we don't have all data, error
            if (bitReservoirCurrentFrameIndex == 0) {
                // Indicate that result is incorrect
                result = false;

                // Read first frame header
                bitReservoirCurrentFrameHeader = tryToGetFrameHeaderAtIndex(0);

                // We start after invalid data
                bytesReadStart = beginOffset;//TODO: voir si ok

                // Exit loop
                break;
            }

            // Go to previous frame
            --bitReservoirCurrentFrameIndex;
            bitReservoirCurrentFrameHeader = tryToGetFrameHeaderAtIndex(bitReservoirCurrentFrameIndex);

            // Check if we have enough data
            if ((*bitReservoirCurrentFrameHeader).getDataSize() >= beginOffset) {
                // Exit loop
                break;
            }

            // Substract bitReservoir current frame data size from offset
            beginOffset -= (*bitReservoirCurrentFrameHeader).getDataSize();
        }

        // Loop to read data
        for (unsigned int bytesRead = bytesReadStart; bytesRead < frameData.size();) {
            // Set position of stream to data of bitReservoir current frame
            _inputStream->seekg((*bitReservoirCurrentFrameHeader).getCRCSize() + (*bitReservoirCurrentFrameHeader).getSideInformationSize() + (((bitReservoirCurrentFrameIndex < frameIndex) && (bytesRead == 0)) ? ((*bitReservoirCurrentFrameHeader).getDataSize() - beginOffset) : 0), std::ios::cur);

            // Calculate available bytes
            unsigned int const limitToRead = ((bitReservoirCurrentFrameIndex < frameIndex) && (bytesRead == 0)) ? beginOffset : (*bitReservoirCurrentFrameHeader).getDataSize();
            unsigned int const bytesAvailable = ((frameData.size() - bytesRead) > limitToRead) ? limitToRead : (frameData.size() - bytesRead);

            // Read available bytes
            _inputStream->read(reinterpret_cast<char *>(&frameData.data()[bytesRead]), bytesAvailable);
            bytesRead += bytesAvailable;

            // Go to next frame
            ++bitReservoirCurrentFrameIndex;
            bitReservoirCurrentFrameHeader = tryToGetFrameHeaderAtIndex(bitReservoirCurrentFrameIndex);

            // Get possible ancillary data
            /*if (bytesRead >= frameData.size()) {//TODO: terminer mais probleme, il faut le side information de bitReservoirCurrentFrameHeader et donc il faut faire tout le meme code qu'il y a dans getFrameAtIndex, donc refactoriser ce code pour le reutiliser ici d'abord !
            }*/
        }

        return result;
    }

    uint16_t Decoder::getCRCIfExist(Frame::Header const &frameHeader) {
        // If no CRC protected, exit
        if (frameHeader.isCRCProtected() == false) {
            return 0;
        }

        // Get stored CRC
        uint16_t storedCRC;
        _inputStream->read(reinterpret_cast<char *>(&storedCRC), frameHeader.getCRCSize());

        // Convert in little endianness
        storedCRC = Helper::revertEndianness(storedCRC);

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
        auto calculatedCRC = Helper::calculateCRC<uint16_t, crcPolynomial, crcInitialValue>(data);

        // Return it
        return calculatedCRC;
    }

    std::optional<Frame::Header> Decoder::tryToGetFrameHeaderAtIndex(unsigned int const frameIndex) {
        // Reset input stream
        _inputStream->clear();

        // Create headerData
        std::optional<std::array<uint8_t, Frame::Header::headerSize>> headerData;

        // Need to search it if not already in array
        if (_frameEntries.size() <= frameIndex) {
            for (unsigned int currentFrameIndex = _frameEntries.size(); currentFrameIndex <= frameIndex; ++currentFrameIndex) {
                // Set stream cursor to beginning of current frame
                _inputStream->seekg((currentFrameIndex > 0) ? (_frameEntries[currentFrameIndex - 1].positionInBytes + _frameEntries[currentFrameIndex - 1].sizeInBytes) : 0);

                // Try to synchronize to next frame
                headerData = tryToReadNextFrameHeaderData((*_inputStream));

                // If frame not found, exit
                if (headerData.has_value() == false) {
                    return std::nullopt;
                }

                // Create header
                Frame::Header header((*headerData), versionMask, versionValue);

                // Save to array
                _frameEntries.push_back({ static_cast<unsigned int>(_inputStream->tellg()) - static_cast<unsigned int>((*headerData).size()), std::max<unsigned int>(header.getFrameLength(), 1) });
            }
        } else {
            // Set default value of optional
            headerData = std::array<uint8_t, Frame::Header::headerSize> {};
        }

        // Set stream cursor to beginning of frame
        _inputStream->seekg(_frameEntries[frameIndex].positionInBytes);

        // Read frame header
        _inputStream->read(reinterpret_cast<char *>((*headerData).data()), (*headerData).size());

        // Create frame header
        return Frame::Header((*headerData), versionMask, versionValue);
    }
    

    Error::DecoderException::DecoderException(Decoder &decoder) :_decoder(decoder) {
    }

    Error::BadMP3Format::BadMP3Format(Decoder &decoder, std::istream &inputStream) : DecoderException(decoder), _inputStream(inputStream) {
    }

    std::istream &Error::BadMP3Format::getInputStream() const {
        return _inputStream;
    }

    Error::FrameDecoderException::FrameDecoderException(Decoder &decoder, unsigned int const frameIndex) : DecoderException(decoder), _frameIndex(frameIndex) {
    }

    unsigned int Error::FrameDecoderException::getFrameIndex() const {
        return _frameIndex;
    }

    Error::FrameNotFound::FrameNotFound(Decoder &decoder, unsigned int const frameIndex) : FrameDecoderException(decoder, frameIndex) {
    }

    Error::FrameCRCIncorrect::FrameCRCIncorrect(Decoder &decoder, unsigned int const frameIndex, uint16_t crcStored, uint16_t crcCalculated, Frame::Header &frameHeader, std::array<uint8_t, Frame::Header::headerSize> const &frameHeaderData, std::vector<uint8_t> &frameSideInformationData) : FrameDecoderException(decoder, frameIndex), _crcStored(crcStored), _crcCalculated(crcCalculated), _frameHeaderTemp(frameHeader), _frameHeader(frameHeader), _frameHeaderData(frameHeaderData), _frameSideInformationData(frameSideInformationData) {
    }

    Error::FrameCRCIncorrect::FrameCRCIncorrect(Decoder &decoder, unsigned int const frameIndex, uint16_t crcStored, uint16_t crcCalculated, Frame::Header const &frameHeader, std::array<uint8_t, Frame::Header::headerSize> const &frameHeaderData, std::vector<uint8_t> const &frameSideInformationData) : FrameDecoderException(decoder, frameIndex), _crcStored(crcStored), _crcCalculated(crcCalculated), _frameHeaderTemp(frameHeader), _frameHeader(_frameHeaderTemp), _frameHeaderData(frameHeaderData), _frameSideInformationDataTemp(frameSideInformationData), _frameSideInformationData(_frameSideInformationDataTemp) {
    }

    uint16_t Error::FrameCRCIncorrect::getCRCStored() const {
        return _crcStored;
    }

    uint16_t Error::FrameCRCIncorrect::getCRCCalculated() const {
        return _crcCalculated;
    }

    Frame::Header &Error::FrameCRCIncorrect::getFrameHeader() const {
        return _frameHeader;
    }

    std::array<uint8_t, Frame::Header::headerSize> const &Error::FrameCRCIncorrect::getFrameHeaderData() const {
        return _frameHeaderData;
    }

    std::vector<uint8_t> &Error::FrameCRCIncorrect::getFrameSideInformationData() const {
        return _frameSideInformationData;
    }

    Error::FrameNoData::FrameNoData(Decoder &decoder, unsigned int const frameIndex, std::vector<uint8_t> &frameData) : FrameDecoderException(decoder, frameIndex), _frameData(frameData) {
    }

    Error::FrameNoData::FrameNoData(Decoder &decoder, unsigned int const frameIndex, std::vector<uint8_t> const &frameData) : FrameDecoderException(decoder, frameIndex), _frameDataTemp(frameData), _frameData(_frameDataTemp) {
    }

    std::vector<uint8_t> &Error::FrameNoData::getFrameData() const {
        return _frameData;
    }
    
}
