
#include <stdexcept>
#include "Header.hpp"
#include "Data/Header.hpp"


namespace MP3::Frame {

    bool Header::isValidHeader(std::array<uint8_t, headerSize> const &headerBytes, uint8_t const versionMask, uint8_t const versionValue) {
        // Check first byte
        if (headerBytes[0] != 0xFF) {
            return false;
        }

        // Check second byte
        if ((headerBytes[1] & versionMask) != versionValue) {
            return false;
        }

        // Header is valid
        return true;
    }

    Header::Header(std::array<uint8_t, headerSize> const &headerBytes, uint8_t const versionMask, uint8_t const versionValue) {
        // Check if headerBytes is correct
        if (isValidHeader(headerBytes, versionMask, versionValue) == false) {
            throw std::invalid_argument("headerBytes is incorrect");  // TODO: changer par une classe custom
        }

        // Decode header bytes
        decodeHeaderBytes(headerBytes);
    }

    unsigned int Header::getFrameSize() const {
        return 1152; //TODO: a mettre dans un const ? et voir par rapport aux autres layers que 3
    }

    unsigned int Header::getFrameLength() const {
        if (Data::samplingRates[_samplingRateIndex] == 0) {
            return 0;
        }

        return ((144000 * _bitrate) / Data::samplingRates[_samplingRateIndex]) + ((_isPadded) ? 1 : 0);
    }

    unsigned int Header::getCRCSize() const {
        return (isCRCProtected() == true) ? 2 : 0;
    }

    unsigned int Header::getSideInformationSize() const {
        return (getNumberOfChannels() == 1) ? 17 : 32;
    }

    unsigned int Header::getDataSize() const {
        // Data size is total size minus header and crc if exist and side informations
        return getFrameLength() - (headerSize + getCRCSize() + getSideInformationSize());
    }

    unsigned int Header::getNumberOfChannels() const {
        return (_channelMode == ChannelMode::SingleChannel) ? 1 : 2;
    }

    unsigned int Header::getBitrate() const {
        return _bitrate;
    }

    unsigned int Header::getSamplingRateIndex() const {
        return _samplingRateIndex;
    }

    ChannelMode Header::getChannelMode() const {
        return _channelMode;
    }

    Emphasis Header::getEmphasis() const {
        return _emphasis;
    }

    bool Header::isPadded() const {
        return _isPadded;
    }

    unsigned int Header::getPrivateBit() const {
        return _privateBit;
    }

    bool Header::isCRCProtected() const {
        return _isCRCProtected;
    }

    bool Header::isIntensityStereo() const {
        return _isIntensityStereo;
    }

    bool Header::isMSStereo() const {
        return _isMSStereo;
    }

    bool Header::isCopyrighted() const {
        return _isCopyrighted;
    }

    bool Header::isOriginal() const {
        return _isOriginal;
    }

    void Header::decodeHeaderBytes(std::array<uint8_t, headerSize> const &headerBytes) {
        //TODO: voir si pas plutot utiliser getBitAtIndex et rendre getBitAtIndex plus générique avec le parametre data en template parameter
        // Set isCRCProtected
        _isCRCProtected = (headerBytes[1] & 0x1) == 0x0;

        // Set bitrate
        _bitrate = Data::bitrates[headerBytes[2] >> 4];

        // Set sampling rate index
        _samplingRateIndex = (headerBytes[2] >> 2) & 0x3;

        // Set padding bit
        _isPadded = (headerBytes[2] & 0x2) == 0x2;

        // Set private bit
        _privateBit = headerBytes[2] & 0x1;

        // Set channel mode
        _channelMode = static_cast<ChannelMode>(headerBytes[3] >> 6);

        // Set MS Stereo and Intensity Stereo
        _isMSStereo = (headerBytes[3] & 0x20) == 0x20;
        _isIntensityStereo = (headerBytes[3] & 0x10) == 0x10;

        // Set copyright
        _isCopyrighted = (headerBytes[3] & 0x8) == 0x8;

        // Set original
        _isOriginal = (headerBytes[3] & 0x4) == 0x4;

        // Set emphasis
        _emphasis = static_cast<Emphasis>(headerBytes[3] & 0x3);
    }

}
