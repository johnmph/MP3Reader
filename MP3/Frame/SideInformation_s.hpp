#ifndef MP3_FRAME_SIDEINFORMATION_S_HPP
#define MP3_FRAME_SIDEINFORMATION_S_HPP


template <class TFunction>
SideInformation::SideInformation(Header const &header, std::vector<uint8_t> const &data, TFunction &&errorFunction) : _header(header), _data(data) {
    // Resize granules vector (always 2 granules but variable number of channels)
    _granules.resize(2 * _header.getNumberOfChannels());

    // Resize scaleFactorShare vector
    _scaleFactorShare.resize(4 * _header.getNumberOfChannels());

    // Extract
    extract(std::forward<TFunction>(errorFunction));
}

template <class TFunction>
void SideInformation::extract(TFunction &&errorFunction) {
    // Get number of channels
    unsigned int const nbrChannels = _header.getNumberOfChannels();

    unsigned int dataBitIndex = 0;

    // Get mainDataBegin
    _mainDataBegin = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 9);

    // Get privateBits
    _privateBits = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, (nbrChannels == 1) ? 5 : 3);

    // Get scaleFactor
    for (unsigned int channelIndex = 0; channelIndex < nbrChannels; ++channelIndex) {
        for (unsigned int scaleFactorShareIndex = 0; scaleFactorShareIndex < 4; ++scaleFactorShareIndex) {
            _scaleFactorShare[(channelIndex * 4) + scaleFactorShareIndex] = Helper::getBitsAtIndex<bool>(_data, dataBitIndex, 1);
        }
    }

    // Get granules
    for (unsigned int granuleIndex = 0; granuleIndex < 2; ++granuleIndex) {
        for (unsigned int channelIndex = 0; channelIndex < nbrChannels; ++channelIndex) {
            _granules[(granuleIndex * nbrChannels) + channelIndex] = extractGranule(dataBitIndex, std::forward<TFunction>(errorFunction));
        }

        // In MS Stereo both channels must have the same block type
        if ((_header.getChannelMode() == ChannelMode::JointStereo) && (_header.isMSStereo() == true) && (_granules[(granuleIndex * 2) + 0].blockType != _granules[(granuleIndex * 2) + 1].blockType)) {//TODO: voir pour la condition si on change isMSStereo pour tester aussi JointStereo alors plus besoin de la 1ere condition
            if (errorFunction(Error::MSStereoDifferentBlockType(*this, _granules[(granuleIndex * 2) + 0].blockType, _granules[(granuleIndex * 2) + 1].blockType)) == false) {
                throw Error::MSStereoDifferentBlockType(*this, _granules[(granuleIndex * 2) + 0].blockType, _granules[(granuleIndex * 2) + 1].blockType);
            }
        }
    }
}

template <class TFunction>
SideInformationGranule SideInformation::extractGranule(unsigned int &dataBitIndex, TFunction &&errorFunction) {
    SideInformationGranule sideInformationGranule;

    // Get par23Length
    sideInformationGranule.par23Length = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 12);
        
    // Get bigValues
    sideInformationGranule.bigValues = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 9);

    // Get globalGain
    sideInformationGranule.globalGain = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 8);

    // Get scaleFacCompress
    auto const scaleFactorCompressIndex = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 4);

    sideInformationGranule.sLen1 = Data::scaleFactorCompress[scaleFactorCompressIndex][0];
    sideInformationGranule.sLen2 = Data::scaleFactorCompress[scaleFactorCompressIndex][1];

    // Get windowSwitchingFlag
    sideInformationGranule.windowSwitchingFlag = Helper::getBitsAtIndex<bool>(_data, dataBitIndex, 1);

    // Check windowSwitchingFlag
    if (sideInformationGranule.windowSwitchingFlag == true) {
        // Initialize window variant to Special window
        sideInformationGranule.window = SideInformationGranuleSpecialWindow {};

        // Get blockType
        sideInformationGranule.blockType = static_cast<BlockType>(Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 2));

        // Block type can't be 0 if window switching flag is set
        if (sideInformationGranule.blockType == BlockType::Normal) {
            if (errorFunction(Error::WindowSwitchingFlagWithNormalBlock(*this, sideInformationGranule.blockType)) == false) {
                throw Error::WindowSwitchingFlagWithNormalBlock(*this, sideInformationGranule.blockType);
            }
        }

        // Get mixedBlockFlag
        std::get<SideInformationGranuleSpecialWindow>(sideInformationGranule.window).mixedBlockFlag = Helper::getBitsAtIndex<bool>(_data, dataBitIndex, 1);

        // Get tableSelect
        for (int i = 0; i < 2; ++i) {
            sideInformationGranule.tableSelect.push_back(Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 5));

            // Table 4 and table 14 doesn't exist
            if ((sideInformationGranule.tableSelect[i] == 4) || (sideInformationGranule.tableSelect[i] == 14)) {
                if (errorFunction(Error::InvalidHuffmanTable(*this, i, sideInformationGranule.tableSelect[i])) == false) {
                    throw Error::InvalidHuffmanTable(*this, i, sideInformationGranule.tableSelect[i]);
                }
            }
        }

        // Get subblockGain
        for (int i = 0; i < 3; ++i) {
            std::get<SideInformationGranuleSpecialWindow>(sideInformationGranule.window).subblockGain[i] = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 3);
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
            sideInformationGranule.tableSelect.push_back(Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 5));

            // Table 4 and table 14 doesn't exist
            if ((sideInformationGranule.tableSelect[i] == 4) || (sideInformationGranule.tableSelect[i] == 14)) {
                if (errorFunction(Error::InvalidHuffmanTable(*this, i, sideInformationGranule.tableSelect[i])) == false) {
                    throw Error::InvalidHuffmanTable(*this, i, sideInformationGranule.tableSelect[i]);
                }
            }
        }

        // Get region0Count
        sideInformationGranule.region0Count = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 4);

        // Get region1Count
        sideInformationGranule.region1Count = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 3);
    }

    // Get preflag
    sideInformationGranule.preflag = Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 1);

    // Get scaleFacScale
    sideInformationGranule.scaleFacScale = Data::scaleFactorScale[Helper::getBitsAtIndex<unsigned int>(_data, dataBitIndex, 1)];

    // Get count1TableIsB
    sideInformationGranule.isCount1TableB = Helper::getBitsAtIndex<bool>(_data, dataBitIndex, 1);

    return sideInformationGranule;
}

#endif /* MP3_FRAME_SIDEINFORMATION_S_HPP */
