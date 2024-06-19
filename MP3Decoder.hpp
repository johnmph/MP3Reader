#ifndef MP3DECODER_HPP
#define MP3DECODER_HPP

#include <istream>
#include <vector>
#include <unordered_map>

//TODO: par apres, créer une classe Frame qui contiendra toutes les informations nécessaires et calculera les scalefactors, les données huffman decodées, ... mais le bit reservoir lui sera a part, dans le decoder et le decoder passera le bit reservoir en parametre dans les methodes d'extraction de la classe Frame
namespace MP3 {

    // Enum
    enum class ChannelMode {
        Stereo = 0,
        JointStereo,
        DualChannel,
        SingleChannel
    };

    enum class WindowType {
        Forbidden = 0,
        Start,
        ShortWindows3,
        End
    };

    struct FrameHeader {
        int bitrate;
        int samplingRateIndex;
        ChannelMode channelMode;
        int emphasis;
        bool isPadded;
        bool crcProtection;
        bool intensityStereo;
        bool MSStereo;
        bool isCopyrighted;
        bool isOriginal;
    };

    struct SideInformationGranuleNormalWindow {//TODO: a changer
    };

    struct SideInformationGranuleSpecialWindow {
        WindowType windowType;
        bool mixedBlockFlag;
        int subblockGain[3];
    };

    union SideInformationGranuleWindow {    // TODO: union ou std::variant ?
        SideInformationGranuleNormalWindow normal;
        SideInformationGranuleSpecialWindow special;
    };

    struct SideInformationGranule {//TODO: bien regarder aux types (unsigned int a la place d'int pour les tailles)
        int par23Length;
        int bigValues;
        int globalGain;
        int sLen1;
        int sLen2;
        std::vector<int> tableSelect;
        int region0Count;
        int region1Count;
        bool windowSwitchingFlag;
        SideInformationGranuleWindow window;
        int preflag;
        float scaleFacScale;
        bool count1TableIsB;
    };

    struct SideInformation {
        int mainDataBegin;
        int privateBits;
        bool scaleFactorShare[2][4];
        std::vector<SideInformationGranule> granules[2];
    };

    struct ScaleFactors {
        int shortWindow[12][3];
        int longWindow[21];
    };
    
    struct Decoder {

        FrameHeader getNextFrame(std::istream &inputStream);
        int getFrameSize(FrameHeader const &frameHeader) const;
        
    private:

        template <class TValueType>
        TValueType decodeHuffmanCode(std::vector<uint8_t> const &data, unsigned int &bitIndex, std::unordered_map<unsigned int, TValueType> const &huffmanTable) const;

        template <class TScaleFactorBandTable>
        int getScaleFactorBandIndexForFrequencyLine(TScaleFactorBandTable const &scaleFactorBands, int frequencyLine) const;

        int correctHuffmanDecodedValueSign(std::vector<uint8_t> const &data, unsigned int &bitIndex, int value) const;
        int correctHuffmanDecodedValue(std::vector<uint8_t> const &data, unsigned int &bitIndex, int value, unsigned int linbits) const;

        std::vector<uint8_t> getDataFromStream(std::istream &inputStream, unsigned int sizeInBits) const;
        ScaleFactors readScaleFactors(std::vector<uint8_t> const &data, unsigned int &bitIndex, SideInformationGranule const &sideInformationGranule, bool const (&scaleFactorShare)[4], bool isFirstGranule) const;
        int getRegionForFrequencyLine(SideInformationGranule const &sideInformationGranule, int frequencyLine, int samplingRateIndex) const;
        std::vector<int> readHuffmanCode(std::vector<uint8_t> const &data, unsigned int &bitIndex, SideInformationGranule const &sideInformationGranule, int samplingRateIndex) const;
        int getScaleFactorShareGroupForScaleFactorBand(int scaleFactorBand) const;
        int getNumberOfChannels(FrameHeader const &frameHeader) const;
        FrameHeader getNextFrameHeader(std::istream &inputStream) const;
        bool synchronizeToNextFrame(std::istream &inputStream, uint8_t versionMask, uint8_t versionValue, bool &hasCRC) const;
        SideInformation readSideInformation(std::istream &inputStream, FrameHeader const &frameHeader) const;
        int getBitsAtIndex(std::vector<uint8_t> const &data, unsigned int &index, unsigned int size) const;
        SideInformationGranule readSideInformationGranule(std::vector<uint8_t> const &data, unsigned int &bitIndex) const;
        ScaleFactors readMainData(std::istream &inputStream, FrameHeader const &frameHeader, SideInformation const &sideInformation);

        std::vector<int> requantize(std::vector<int> const &values, ScaleFactors const &scaleFactors, SideInformationGranule const &sideInformationGranule, int samplingRateIndex) const;

        std::vector<uint8_t> _bitReservoir;
    };

}

#endif /* MP3DECODER_HPP */
