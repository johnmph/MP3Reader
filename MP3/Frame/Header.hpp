#ifndef MP3_FRAME_HEADER_HPP
#define MP3_FRAME_HEADER_HPP

#include <cstdint>
#include <array>


namespace MP3::Frame {

    // Enum
    enum class ChannelMode {
        Stereo = 0,
        JointStereo,
        DualChannel,
        SingleChannel
    };

    enum class Emphasis {
        None = 0,
        Ms50_15,
        Reserved,
        CCITT_J17
    };

    struct Header {

        static constexpr unsigned int headerSize = 4;

        static bool isValidHeader(std::array<uint8_t, headerSize> const &data, uint8_t const versionMask, uint8_t const versionValue);

        Header(std::array<uint8_t, headerSize> const &data, uint8_t const versionMask, uint8_t const versionValue);

        std::array<uint8_t, headerSize> const &getData() const;
        unsigned int getFrameSize() const;
        unsigned int getFrameLength() const;
        unsigned int getCRCSize() const;
        unsigned int getSideInformationSize() const;
        unsigned int getDataSize() const;
        unsigned int getNumberOfChannels() const;
        unsigned int getBitrateIndex() const;
        unsigned int getBitrate() const;
        unsigned int getSamplingRateIndex() const;
        unsigned int getSamplingRate() const;
        ChannelMode getChannelMode() const;
        Emphasis getEmphasis() const;
        bool isPadded() const;
        unsigned int getPrivateBit() const;
        bool isCRCProtected() const;
        bool isIntensityStereo() const;
        bool isMSStereo() const;
        bool isCopyrighted() const;
        bool isOriginal() const;

    private:
        void decode();//TODO: mettre un nom qui est du meme genre que dans SideInformation et Frame

        std::array<uint8_t, headerSize> _data;
        unsigned int _bitrateIndex;
        unsigned int _samplingRateIndex;
        ChannelMode _channelMode;
        Emphasis _emphasis;
        bool _isPadded;
        unsigned int _privateBit;
        bool _isCRCProtected;
        bool _isIntensityStereo;
        bool _isMSStereo;
        bool _isCopyrighted;
        bool _isOriginal;
    };
}

#endif /* MP3_FRAME_HEADER_HPP */
