
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

        // TODO: est ce une erreur d'avoir ms stereo ou intensity stereo set alors qu'on est pas en joint stereo ou faut il juste ignorer ?
        
        // Header is valid
        return true;
    }

    Header::Header(std::array<uint8_t, headerSize> const &data, uint8_t const versionMask, uint8_t const versionValue) : _data(data) {
        assert(isValidHeader(data, versionMask, versionValue));
        
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
        return ((144000 * getBitrate()) / getSamplingRate()) + ((_isPadded) ? 1 : 0);//TODO: si retourne 0 car mauvais header (qui ne peut arriver que si on retire les tests des sampling rate / bit rate dans le isValidHeader pour les mettre dans le decode), faut t'il retourner 1 ? (car ainsi on avance meme si le header est mauvais) ou bien laisser ainsi et avancer dans le decoder partout ou on appelle getFrameLength ??
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

    unsigned int Header::getBitrateIndex() const {
        return _bitrateIndex;
    }

    unsigned int Header::getBitrate() const {
        return Data::bitrates[_bitrateIndex];
    }

    unsigned int Header::getSamplingRateIndex() const {
        return _samplingRateIndex;
    }

    unsigned int Header::getSamplingRate() const {
        return Data::samplingRates[_samplingRateIndex];
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
        unsigned int dataBitIndex = 15;

        // Set isCRCProtected
        _isCRCProtected = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 1) == 0x0;

        // Set bitrate index
        _bitrateIndex = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 4);

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

        // Set copyright
        _isCopyrighted = Helper::getBitsAtIndex<bool>(_data, dataBitIndex, 1);

        // Set original
        _isOriginal = Helper::getBitsAtIndex<bool>(_data, dataBitIndex, 1);

        // Set emphasis
        _emphasis = static_cast<Emphasis>(Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 2));
    }

}
