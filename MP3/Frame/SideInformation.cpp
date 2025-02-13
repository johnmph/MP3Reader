
#include "SideInformation.hpp"


namespace MP3::Frame {

    std::vector<uint8_t> const &SideInformation::getData() const {
        return _data;
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


    Error::SideInformationException::SideInformationException(SideInformation &sideInformation) :_sideInformation(sideInformation) {
    }

    Error::MSStereoDifferentBlockType::MSStereoDifferentBlockType(SideInformation &sideInformation, BlockType &blockTypeGranuleLeft, BlockType &blockTypeGranuleRight) : SideInformationException(sideInformation), _blockTypeGranuleLeft(blockTypeGranuleLeft), _blockTypeGranuleRight(blockTypeGranuleRight) {
    }

    BlockType &Error::MSStereoDifferentBlockType::getBlockTypeGranuleLeft() const {
        return _blockTypeGranuleLeft;
    }

    BlockType &Error::MSStereoDifferentBlockType::getBlockTypeGranuleRight() const {
        return _blockTypeGranuleRight;
    }
    
    Error::WindowSwitchingFlagWithNormalBlock::WindowSwitchingFlagWithNormalBlock(SideInformation &sideInformation, BlockType &blockType) : SideInformationException(sideInformation), _blockType(blockType) {
    }

    BlockType &Error::WindowSwitchingFlagWithNormalBlock::getBlockType() const {
        return _blockType;
    }

    Error::InvalidHuffmanTable::InvalidHuffmanTable(SideInformation &sideInformation, unsigned int tableIndex, unsigned int &table) : SideInformationException(sideInformation), _tableIndex(tableIndex), _table(table) {
    }

    unsigned int Error::InvalidHuffmanTable::getTableIndex() const {
        return _tableIndex;
    }

    unsigned int &Error::InvalidHuffmanTable::getTable() const {
        return _table;
    }
    
}
