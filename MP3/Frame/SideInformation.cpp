
#include "SideInformation.hpp"
#include "../Helper.hpp"
#include "Data/SideInformation.hpp"


namespace MP3::Frame {

    SideInformation::SideInformation(Header const &header, std::vector<uint8_t> const &data) : _header(header) {
        // Resize granules vector (always 2 granules but variable number of channels)
        _granules.resize(2 * _header.getNumberOfChannels());

        // Resize scaleFactorShare vector
        _scaleFactorShare.resize(4 * _header.getNumberOfChannels());

        // Extract
        extract(data);
    }

    unsigned int SideInformation::getMainDataBegin() const {
        return _mainDataBegin;
    }

    unsigned int SideInformation::getMainDataSizeInBits() const {
        unsigned int sizeInBits = 0;

        // Browse granules
        for (unsigned int granuleIndex = 0; granuleIndex < 2; ++granuleIndex) {
            // Browse channels
            for (unsigned int channelIndex = 0; channelIndex < _header.getNumberOfChannels(); ++channelIndex) {
                sizeInBits += _granules[(granuleIndex * _header.getNumberOfChannels()) + channelIndex].par23Length;
            }
        }

        return sizeInBits;
    }

    unsigned int SideInformation::getPrivateBits() const {
        return _privateBits;
    }

    bool SideInformation::getScaleFactorShare(unsigned int const channelIndex, unsigned int const scaleFactorShareIndex) const {
        // TODO: assert sur channelIndex ?
        return _scaleFactorShare[(channelIndex * 4) + scaleFactorShareIndex];
    }
    
    SideInformationGranule const &SideInformation::getGranule(unsigned int const granuleIndex, unsigned int const channelIndex) const {
        //TODO: assert sur channelIndex ?
        return _granules[(granuleIndex * _header.getNumberOfChannels()) + channelIndex];
    }

    void SideInformation::extract(std::vector<uint8_t> const &data) {
        // Get number of channels
        unsigned int const nbrChannels = _header.getNumberOfChannels();

        unsigned int dataBitIndex = 0;

        // Get mainDataBegin
        _mainDataBegin = Helper::getBitsAtIndex(data, dataBitIndex, 9);//TODO: verifier si c'est ok niveau endian et data sens, verifier si positif ou negatif

        // Get privateBits
        _privateBits = Helper::getBitsAtIndex(data, dataBitIndex, (nbrChannels == 1) ? 5 : 3);

        // Get scaleFactor
        for (unsigned int channelIndex = 0; channelIndex < nbrChannels; ++channelIndex) {
            for (unsigned int scaleFactorShareIndex = 0; scaleFactorShareIndex < 4; ++scaleFactorShareIndex) {
                _scaleFactorShare[(channelIndex * 4) + scaleFactorShareIndex] = Helper::getBitsAtIndex(data, dataBitIndex, 1);
            }
        }

        // Get granules
        for (unsigned int granuleIndex = 0; granuleIndex < 2; ++granuleIndex) {
            for (unsigned int channelIndex = 0; channelIndex < nbrChannels; ++channelIndex) {
                _granules[(granuleIndex * nbrChannels) + channelIndex] = extractGranule(data, dataBitIndex);
            }
        }
    }

    SideInformationGranule SideInformation::extractGranule(std::vector<uint8_t> const &data, unsigned int &dataBitIndex) {
        SideInformationGranule sideInformationGranule;

        // Get par23Length
        sideInformationGranule.par23Length = Helper::getBitsAtIndex(data, dataBitIndex, 12);
        
        // Get bigValues
        sideInformationGranule.bigValues = Helper::getBitsAtIndex(data, dataBitIndex, 9);

        // Get globalGain
        sideInformationGranule.globalGain = Helper::getBitsAtIndex(data, dataBitIndex, 8);

        // Get scaleFacCompress
        auto const scaleFactorCompressIndex = Helper::getBitsAtIndex(data, dataBitIndex, 4);

        sideInformationGranule.sLen1 = Data::scaleFactorCompress[scaleFactorCompressIndex][0];
        sideInformationGranule.sLen2 = Data::scaleFactorCompress[scaleFactorCompressIndex][1];

        // Get windowSwitchingFlag
        sideInformationGranule.windowSwitchingFlag = Helper::getBitsAtIndex(data, dataBitIndex, 1) != 0;

        // Check windowSwitchingFlag
        if (sideInformationGranule.windowSwitchingFlag == true) {
            // Initialize window variant to Special window
            sideInformationGranule.window = SideInformationGranuleSpecialWindow {};

            // Get blockType
            sideInformationGranule.blockType = static_cast<BlockType>(Helper::getBitsAtIndex(data, dataBitIndex, 2));

            // Get mixedBlockFlag
            std::get<SideInformationGranuleSpecialWindow>(sideInformationGranule.window).mixedBlockFlag = Helper::getBitsAtIndex(data, dataBitIndex, 1) != 0;

            // Get tableSelect
            for (int i = 0; i < 2; ++i) {
                sideInformationGranule.tableSelect.push_back(Helper::getBitsAtIndex(data, dataBitIndex, 5));
            }

            // Get subblockGain
            for (int i = 0; i < 3; ++i) {
                std::get<SideInformationGranuleSpecialWindow>(sideInformationGranule.window).subblockGain[i] = Helper::getBitsAtIndex(data, dataBitIndex, 3);
            }

            // region0Count and region1Count are set to fixed values if short block
            sideInformationGranule.region0Count = ((sideInformationGranule.blockType == BlockType::ShortWindows3) && (std::get<SideInformationGranuleSpecialWindow>(sideInformationGranule.window).mixedBlockFlag == false)) ? 8 : 7;
            sideInformationGranule.region1Count = 36;
        } else {
            // Initialize window variant to Normal window
            sideInformationGranule.window = SideInformationGranuleNormalWindow {};

            // Set blockType to normal
            sideInformationGranule.blockType = BlockType::Normal;

            // Get tableSelect
            for (int i = 0; i < 3; ++i) {
                sideInformationGranule.tableSelect.push_back(Helper::getBitsAtIndex(data, dataBitIndex, 5));
            }

            // Get region0Count
            sideInformationGranule.region0Count = Helper::getBitsAtIndex(data, dataBitIndex, 4);

            // Get region1Count
            sideInformationGranule.region1Count = Helper::getBitsAtIndex(data, dataBitIndex, 3);
        }

        // Get preflag
        sideInformationGranule.preflag = Helper::getBitsAtIndex(data, dataBitIndex, 1);

        // Get scaleFacScale
        sideInformationGranule.scaleFacScale = Data::scaleFactorScale[Helper::getBitsAtIndex(data, dataBitIndex, 1)];

        // Get count1TableIsB
        sideInformationGranule.isCount1TableB = Helper::getBitsAtIndex(data, dataBitIndex, 1) != 0;

        return sideInformationGranule;
    }
    
}
