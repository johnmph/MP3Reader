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
        Frame(Header const &header, SideInformation const &sideInformation, unsigned int ancillaryDataSizeInBits, std::vector<uint8_t> const &data);//TODO: voir si rvalue, ET SURTOUT PASSER data avec la gestion du bit reservoir

        //unsigned int get //TODO: retourner dataBitsIndex ? pour vérifier la taille ? 
        std::vector<uint8_t> const &getAncillaryBits() const;//TODO: a voir aussi pour la taille si la get aussi
        std::array<float, 576> const &getPCMSamples(unsigned int const granuleIndex, unsigned int const channelIndex) const;

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
        void applyOverlapping(std::array<float, 36> const &subbandValues, std::array<float, 576> &currentSubbandsValues, unsigned int const subbandIndex);
        void applyCompensationForFrequencyInversion(std::array<float, 576> &currentSubbandsValues, unsigned int const subbandIndex);
        void applyPolyphaseFilterBank(unsigned int const granuleIndex, unsigned int const channelIndex);


        Header const _header;
        SideInformation const _sideInformation;
        std::vector<uint8_t> const _data;
        unsigned int _dataBitIndex;
        std::vector<ScaleFactors> _scaleFactors;
        std::vector<std::array<float, 576>> _frequencyLineValues; // TODO: int ou float ? -> normalement float
        std::vector<std::array<float, 576>> _subbandsValues;
        std::vector<std::array<float, 576>> _pcmValues;//TODO: voir si diviser par granules aussi !!!
        std::vector<unsigned int> _startingRzeroFrequencyLineIndexes;   //TODO: par apres voir si regrouper avec les 2 du dessus dans une structure et avoir un std::vector<Structure>

        static std::vector<std::array<float, 1024>> _shiftedAndMatrixedSubbandsValues;
    };

}

#endif /* MP3_FRAME_FRAME_HPP */
