
#include <stdexcept>
#include "Header.hpp"
#include "Data/Header.hpp"
#include "../Helper.hpp"


namespace MP3::Frame {

    bool Header::isValidHeader(std::array<uint8_t, headerSize> const &data, uint8_t const versionMask, uint8_t const versionValue) {
        // Check first byte
        if (data[0] != 0xFF) {
            return false;
        }

        // Check second byte
        if ((data[1] & versionMask) != versionValue) {
            return false;
        }

        // Bitrate index can't be 0 or 15 TODO: voir pour 0 car c'est le mode free
        if (((data[2] & 0xF0) == 0x0) || ((data[2] & 0xF0) == 0xF0)) {
            return false;
        }

        // Sampling rate index can't be 3 (reserved)
        if ((data[2] & 0x0C) == 0x0C) {
            return false;
        }

        // Header is valid
        return true;
    }

    Header::Header(std::array<uint8_t, headerSize> const &data, uint8_t const versionMask, uint8_t const versionValue) : _data(data) {
        // Check if data is correct
        if (isValidHeader(data, versionMask, versionValue) == false) {
            throw std::invalid_argument("Data is incorrect");  // TODO: changer par une classe custom
        }

        // Decode data
        decode();
    }

    std::array<uint8_t, Header::headerSize> const &Header::getData() const {
        return _data;
    }

    unsigned int Header::getFrameSize() const {
        return 1152; //TODO: a mettre dans un const ? et voir par rapport aux autres layers que 3
    }

    unsigned int Header::getFrameLength() const {
        if (Data::samplingRates[_samplingRateIndex] == 0) {
            return 0;//TODO: probleme car on ne va pas avancer la tete de lecture quand on recherchera la prochaine frame
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

    void Header::decode() {
        unsigned int dataBitIndex = 15;//TODO: pour tester on a mis 14 et 16 et on est dans une boucle infinie ? a verifier car si fichier mal formé on peut bloquer ?

        // Set isCRCProtected
        _isCRCProtected = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 1) == 0x0;

        // Set bitrate
        _bitrate = Data::bitrates[Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 4)];//TODO: voir si sauver bitrate ou bitrateIndex pour rester consistent avec samplingrateindex

        // Set sampling rate index
        _samplingRateIndex = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 2);

        // Set padding bit
        _isPadded = Helper::getBitsAtIndex<bool>(_data, dataBitIndex, 1);

        // Set private bit
        _privateBit = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 1);

        // Set channel mode
        _channelMode = static_cast<ChannelMode>(Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 2));

        // Set MS Stereo and Intensity Stereo
        _isMSStereo = Helper::getBitsAtIndex<bool>(_data, dataBitIndex, 1);
        _isIntensityStereo = Helper::getBitsAtIndex<bool>(_data, dataBitIndex, 1);

        // TODO: est ce une erreur d'avoir ms stereo ou intensity stereo set alors qu'on est pas en joint stereo ou faut il juste ignorer ?

        // Set copyright
        _isCopyrighted = Helper::getBitsAtIndex<bool>(_data, dataBitIndex, 1);

        // Set original
        _isOriginal = Helper::getBitsAtIndex<bool>(_data, dataBitIndex, 1);

        // Set emphasis
        _emphasis = static_cast<Emphasis>(Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 2));
    }

}
