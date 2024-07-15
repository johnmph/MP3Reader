#ifndef MP3_FRAME_FRAME_HPP
#define MP3_FRAME_FRAME_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <unordered_map>
#include "Header.hpp"
#include "SideInformation.hpp"


namespace MP3::Frame {

    struct ScaleFactors {
        std::array<std::array<unsigned int, 3>, 12> shortWindow;
        std::array<unsigned int, 21> longWindow;
    };

    struct Frame {

        // TODO: avoir une methode pour soit renvoyer les PCM samples (getPCMSamples), soit une methode browsePCMSamples a qui on passe une fonction
        // TODO: avoir un getter pour savoir combien de bits a vraiment besoin cette frame, pour savoir ce que on laisse au bit reservoir
        Frame(Header const &header, SideInformation const &sideInformation, unsigned int ancillaryDataSizeInBits, std::vector<uint8_t> const &data, std::array<std::array<float, 576>, 2> &blocksSubbandsOverlappingValues, std::array<std::array<float, 1024>, 2> &shiftedAndMatrixedSubbandsValues);

        unsigned int getNumberOfChannels() const;
        unsigned int getBitrate() const;
        unsigned int getSamplingRate() const;
        ChannelMode getChannelMode() const;
        bool isCRCProtected() const;
        bool isIntensityStereo() const;
        bool isMSStereo() const;
        bool isCopyrighted() const;
        bool isOriginal() const;

        //unsigned int get //TODO: retourner dataBitsIndex ? pour vérifier la taille ? 
        std::vector<uint8_t> const &getAncillaryBits() const;//TODO: a voir aussi pour la taille si la get aussi
        std::array<float, 1152> const &getPCMSamples(unsigned int const channelIndex) const;


    private:
        void extractMainData();
        void extractScaleFactors(unsigned int const granuleIndex, unsigned int const channelIndex);
        void extractFrequencyLineValues(unsigned int const granuleIndex, unsigned int const channelIndex, unsigned int const granuleStartDataBitIndex);
        void requantizeFrequencyLineValues(unsigned int const granuleIndex, unsigned int const channelIndex);
        void reorderShortWindows(unsigned int const granuleIndex, unsigned int const channelIndex);
        void processStereo(unsigned int const granuleIndex);
        void synthesisFilterBank(unsigned int const granuleIndex, unsigned int const channelIndex);

        unsigned int getScaleFactorShareGroupForScaleFactorBand(unsigned int const scaleFactorBand) const;
        unsigned int getBigValuesRegionForFrequencyLineIndex(SideInformationGranule const &sideInformationGranule, unsigned int const frequencyLineIndex) const;
        
        template <class TValueType>
        TValueType decodeHuffmanCode(unsigned int const granulePart23LengthInBits, unsigned int const granuleStartDataBitIndex, std::unordered_map<unsigned int, TValueType> const &huffmanTable);

        int huffmanCodeApplyLinbitsAndSign(int value, unsigned int const linbits);
        int huffmanCodeApplySign(int value);

        unsigned int getSubblockGainForFrequencyLineIndex(SideInformationGranule const &currentGranule, unsigned int const frequencyLineIndex) const;

        template <class TScaleFactorBandTable>
        unsigned int getScaleFactorBandIndexForFrequencyLineIndex(TScaleFactorBandTable const &scaleFactorBands, unsigned int const frequencyLineIndex) const;

        unsigned int getCurrentWindowIndexForFrequencyLineIndex(unsigned int const scaleFactorBandIndex, unsigned int const frequencyLineIndex) const;
        unsigned int getScaleFactorForFrequencyLineIndex(unsigned int const granuleIndex, unsigned int const channelIndex, SideInformationGranule const &sideInformationGranule, unsigned int const frequencyLineIndex) const;
        bool isFrequencyLineIndexInShortBlock(SideInformationGranule const &sideInformationGranule, unsigned int const frequencyLineIndex) const;
        void processMSStereo(unsigned int const granuleIndex);
        void processIntensityStereo(unsigned int const granuleIndex);
        unsigned int getIntensityStereoBound(unsigned int const granuleIndex) const;
        void applyAliasReduction(unsigned int const granuleIndex, unsigned int const channelIndex);
        void applyIMDCT(SideInformationGranule const &sideInformationGranule, std::array<float, 576> const &frequencyLineValues, std::array<float, 36> &subbandValues, unsigned int const subbandIndex) const;
        void applyWindowing(SideInformationGranule const &sideInformationGranule, std::array<float, 36> &subbandValues, unsigned int const subbandIndex) const;
        void applyOverlapping(unsigned int const channelIndex, std::array<float, 36> const &currentSubbandValues, std::array<float, 576> &subbandsValues, unsigned int const subbandIndex) const;
        void applyCompensationForFrequencyInversion(std::array<float, 576> &subbandsValues, unsigned int const subbandIndex) const;
        void applyPolyphaseFilterBank(unsigned int const granuleIndex, unsigned int const channelIndex, std::array<float, 576> &subbandsValues);


        Header const _header;
        SideInformation const _sideInformation;
        std::vector<uint8_t> const _data;
        unsigned int _dataBitIndex;
        std::array<std::vector<ScaleFactors>, 2> _scaleFactors;
        std::array<std::vector<std::array<float, 576>>, 2> _frequencyLineValues;
        std::vector<std::array<float, 1152>> _pcmValues;
        std::array<std::vector<unsigned int>, 2> _startingRzeroFrequencyLineIndexes;
        std::array<std::array<float, 576>, 2> &_blocksSubbandsOverlappingValues;
        std::array<std::array<float, 1024>, 2> &_shiftedAndMatrixedSubbandsValues;
    };

}

#endif /* MP3_FRAME_FRAME_HPP */
