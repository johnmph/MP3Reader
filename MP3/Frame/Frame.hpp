#ifndef MP3_FRAME_FRAME_HPP
#define MP3_FRAME_FRAME_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <unordered_map>
#include <utility>
#include <optional>
#include "../Helper.hpp"
#include "Data/Frame.hpp"
#include "Data/HuffmanTables.hpp"
#include "Header.hpp"
#include "SideInformation.hpp"


namespace MP3::Frame {

    struct ScaleFactors {
        std::array<std::array<unsigned int, 3>, 12> shortWindow;
        std::array<unsigned int, 21> longWindow;
    };

    struct Frame {

        template <class TFunction>
        Frame(Header const &header, SideInformation const &sideInformation, unsigned int ancillaryDataSizeInBits, std::vector<uint8_t> const &data, std::array<std::array<float, 576>, 2> &blocksSubbandsOverlappingValues, std::array<std::array<float, 1024>, 2> &shiftedAndMatrixedSubbandsValues, TFunction &&errorFunction);

        unsigned int getNumberOfChannels() const;
        unsigned int getBitrate() const;
        unsigned int getSamplingRate() const;
        ChannelMode getChannelMode() const;
        bool isCRCProtected() const;
        bool isIntensityStereo() const;
        bool isMSStereo() const;
        bool isCopyrighted() const;
        bool isOriginal() const;

        std::vector<uint8_t> const &getAncillaryBits() const;//TODO: a voir aussi pour la taille si la get aussi
        std::array<float, 1152> const &getPCMSamples(unsigned int const channelIndex) const;


    private:
        template <class TFunction>
        void extractMainData(TFunction &&errorFunction);

        template <class TFunction>
        void extractFrequencyLineValues(unsigned int const granuleIndex, unsigned int const channelIndex, unsigned int const granuleStartDataBitIndex, TFunction &&errorFunction);

        void extractScaleFactors(unsigned int const granuleIndex, unsigned int const channelIndex);
        void requantizeFrequencyLineValues(unsigned int const granuleIndex, unsigned int const channelIndex);
        void reorderShortWindows(unsigned int const granuleIndex, unsigned int const channelIndex);
        void processStereo(unsigned int const granuleIndex);
        void synthesisFilterBank(unsigned int const granuleIndex, unsigned int const channelIndex);

        unsigned int getScaleFactorShareGroupForScaleFactorBand(unsigned int const scaleFactorBand) const;
        unsigned int getBigValuesRegionForFrequencyLineIndex(SideInformationGranule const &sideInformationGranule, unsigned int const frequencyLineIndex) const;
        
        template <class TValueType>
        std::pair<std::optional<TValueType>, unsigned int> decodeHuffmanCode(unsigned int const granulePart23LengthInBits, unsigned int const granuleStartDataBitIndex, std::unordered_map<unsigned int, TValueType> const &huffmanTable, unsigned int const maxEntryBitLength);

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

    namespace Error {

        struct FrameException : std::exception {
            FrameException(Frame &frame);

            Frame const &getFrame() const;

        private:
            Frame &_frame;//TODO: faire attention avec cette reference car une exception dans le constructor invalide cette reference et donc soit separer error et exception soit ne pas sauver une reference sur frame ici !!!
        };

        template <typename TValue>
        struct HuffmanCodeNotFound : FrameException {
            HuffmanCodeNotFound(Frame &frame, unsigned int huffmanCodedValue, TValue &huffmanDecodedValue, unsigned int frequencyLineIndex);

            unsigned int getHuffmanCodedValue() const;
            TValue &getHuffmanDecodedValue() const;
            unsigned int getFrequencyLineIndex() const;

        private:
            unsigned int const _huffmanCodedValue;
            TValue &_huffmanDecodedValue;
            unsigned int const _frequencyLineIndex;
        };

    }


    #include "Frame_s.hpp"

}

#endif /* MP3_FRAME_FRAME_HPP */
