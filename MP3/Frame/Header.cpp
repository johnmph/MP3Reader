
#include <stdexcept>
#include "Header.hpp"
#include "Data/Header.hpp"
#include "../Helper.hpp"


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
        unsigned int dataBitIndex = 15;//TODO: pour tester on a mis 14 et 16 et on est dans une boucle infinie ? a verifier car si fichier mal formé on peut bloquer ?

        // Set isCRCProtected
        _isCRCProtected = Helper::getBitsAtIndex<unsigned int>(headerBytes, dataBitIndex, 1) == 0x0;

        // Set bitrate
        _bitrate = Data::bitrates[Helper::getBitsAtIndex<unsigned int>(headerBytes, dataBitIndex, 4)];

        // Set sampling rate index
        _samplingRateIndex = Helper::getBitsAtIndex<unsigned int>(headerBytes, dataBitIndex, 2);

        // Set padding bit
        _isPadded = Helper::getBitsAtIndex<bool>(headerBytes, dataBitIndex, 1);

        // Set private bit
        _privateBit = Helper::getBitsAtIndex<unsigned int>(headerBytes, dataBitIndex, 1);

        // Set channel mode
        _channelMode = static_cast<ChannelMode>(Helper::getBitsAtIndex<unsigned int>(headerBytes, dataBitIndex, 2));

        // Set MS Stereo and Intensity Stereo
        _isMSStereo = Helper::getBitsAtIndex<bool>(headerBytes, dataBitIndex, 1);
        _isIntensityStereo = Helper::getBitsAtIndex<bool>(headerBytes, dataBitIndex, 1);

        // Set copyright
        _isCopyrighted = Helper::getBitsAtIndex<bool>(headerBytes, dataBitIndex, 1);

        // Set original
        _isOriginal = Helper::getBitsAtIndex<bool>(headerBytes, dataBitIndex, 1);

        // Set emphasis
        _emphasis = static_cast<Emphasis>(Helper::getBitsAtIndex<unsigned int>(headerBytes, dataBitIndex, 2));
/*
        // Check if bitrate is ok
        if ((_bitrate == 0) || (_bitrate == -1)) {
            //throw ;
        }
*/

    }

}
