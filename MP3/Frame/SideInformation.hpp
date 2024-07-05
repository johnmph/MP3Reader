#ifndef MP3_FRAME_SIDEINFORMATION_HPP
#define MP3_FRAME_SIDEINFORMATION_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <variant>
#include "Header.hpp"


namespace MP3::Frame {

    // Enum
    enum class BlockType : unsigned int {
        Normal = 0,
        Start,
        ShortWindows3,
        End
    };

    struct SideInformationGranuleNormalWindow {
    };

    struct SideInformationGranuleSpecialWindow {
        bool mixedBlockFlag;
        std::array<unsigned int, 3> subblockGain;
    };

    struct SideInformationGranule {
        unsigned int par23Length;
        unsigned int bigValues;
        unsigned int globalGain;
        unsigned int sLen1;
        unsigned int sLen2;
        std::vector<unsigned int> tableSelect;
        unsigned int region0Count;
        unsigned int region1Count;
        bool windowSwitchingFlag;
        BlockType blockType;
        std::variant<SideInformationGranuleNormalWindow, SideInformationGranuleSpecialWindow> window;
        unsigned int preflag;
        float scaleFacScale;
        bool isCount1TableB;
    };

    struct SideInformation {

        SideInformation(Header const &header, std::vector<uint8_t> const &data);

        unsigned int getMainDataBegin() const;
        unsigned int getMainDataSizeInBits() const;
        unsigned int getPrivateBits() const;
        bool getScaleFactorShare(unsigned int const channelIndex, unsigned int const scaleFactorShareIndex) const;
        SideInformationGranule const &getGranule(unsigned int const granuleIndex, unsigned int const channelIndex) const;

    private:
        void extract(std::vector<uint8_t> const &data);
        SideInformationGranule extractGranule(std::vector<uint8_t> const &data, unsigned int &dataBitIndex);

        Header const &_header;
        unsigned int _mainDataBegin;
        unsigned int _privateBits;
        std::vector<bool> _scaleFactorShare;
        std::vector<SideInformationGranule> _granules;
    };

}

#endif /* MP3_FRAME_SIDEINFORMATION_HPP */
