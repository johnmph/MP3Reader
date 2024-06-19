
#include "MP3Decoder.hpp"
#include "HuffmanTables.hpp"
#include "ScaleFactorBandTables.hpp"
#include <cmath>
#include <cassert>


namespace MP3 {

    int const BitrateTable[] = {
        0,
        32,
        40,
        48,
        56,
        64,
        80,
        96,
        112,
        128,
        160,
        192,
        224,
        256,
        320,
        -1
    };

    int const SamplingRateTable[] = {
        44100,
        48000,
        32000,
        0
    };

    int const ScaleFactorCompress[][2] = {
        { 0, 0 },
        { 0, 1 },
        { 0, 2 },
        { 0, 3 },
        { 3, 0 },
        { 1, 1 },
        { 1, 2 },
        { 1, 3 },
        { 2, 1 },
        { 2, 2 },
        { 2, 3 },
        { 3, 1 },
        { 3, 2 },
        { 3, 3 },
        { 4, 2 },
        { 4, 3 }
    };

    float const ScaleFacScale[] = {
        0.5f,
        1.0f
    };

    int const ScaleFactorShareGroupByScaleFactorBand[] = {
        0,
        6,
        11,
        16,
        21
    };

    int const PretabByScaleFactorBand[] = {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        1,
        1,
        1,
        2,
        2,
        3,
        3,
        3,
        2
    };


    FrameHeader Decoder::getNextFrame(std::istream &inputStream) {
        // First get next frame header
        auto frameHeader = getNextFrameHeader(inputStream);

        // TODO: si CRC, le lire
        if (frameHeader.crcProtection) {
            inputStream.seekg(2, std::ios_base::cur);
        }

        // Read Side information
        auto sideInformation = readSideInformation(inputStream, frameHeader);

        // Read main data
        auto mainData = readMainData(inputStream, frameHeader, sideInformation);

        return frameHeader;
    }

    FrameHeader Decoder::getNextFrameHeader(std::istream &inputStream) const {
        FrameHeader frameHeader;
        uint8_t data = 0x0;

        // Synchronize to next frame
        if (synchronizeToNextFrame(inputStream, 0xFE, 0xFA, frameHeader.crcProtection) == false) {
            return frameHeader;//TODO: changer
        }

        // Get third byte
        data = inputStream.get();

        // TODO: voir si inputStream.good() == false

        // Get bitrate
        frameHeader.bitrate = BitrateTable[data >> 4];

        // Get sampling rate
        frameHeader.samplingRateIndex = (data >> 2) & 0x3;

        // Get padding bit
        frameHeader.isPadded = (data & 0x2) == true;

        // Get fourth byte
        data = inputStream.get();

        // TODO: voir si inputStream.good() == false

        // Get channel mode
        frameHeader.channelMode = static_cast<ChannelMode>(data >> 6);

        // Get MS Stereo and Intensity Stereo
        frameHeader.MSStereo = ((data >> 5) & 0x1) == true;
        frameHeader.intensityStereo = ((data >> 4) & 0x1) == true;

        // Get copyright
        frameHeader.isCopyrighted = ((data >> 3) & 0x1) == true;

        // Get original
        frameHeader.isOriginal = ((data >> 2) & 0x1) == true;

        // Get emphasis
        frameHeader.emphasis = (data & 0x3);

        return frameHeader;
    }

    bool Decoder::synchronizeToNextFrame(std::istream &inputStream, uint8_t versionMask, uint8_t versionValue, bool &hasCRC) const {//TODO: a appeler plusieurs fois pour etre sur d'etre bien synchronisé!
        uint8_t data = 0x0;

        // Check for sync
        while (inputStream.good()) {
            // Check for first byte of sync
            for (; (data != 0xFF) && inputStream.good(); data = inputStream.get()) {
            }

            // Check if stream is not good
            if (inputStream.good() == false) {
                break;
            }

            // Check for second byte of sync
            data = inputStream.get();
            if ((data & versionMask) == versionValue) {
                // Set hasCRC
                hasCRC = (data & 0x1) == false;

                // Ok
                return true;
            }
        }

        // Not found
        return false;
    }

    int Decoder::getBitsAtIndex(std::vector<uint8_t> const &data, unsigned int &index, unsigned int size) const {
        assert(size < (sizeof(int) * 8));

        int value = 0;

        for (; size > 0; --size, ++index) {
            value <<= 1;

            if ((index / 8) < data.size()) {
                value |= (data[index / 8] >> (7 - (index % 8))) & 0x1;
            }
        }
        
        return value;
    }

    int Decoder::getNumberOfChannels(FrameHeader const &frameHeader) const {
        return (frameHeader.channelMode == ChannelMode::SingleChannel) ? 1 : 2;// TODO: a voir pour joint stereo
    }

    SideInformation Decoder::readSideInformation(std::istream &inputStream, FrameHeader const &frameHeader) const {
        // Get number of channels
        int nbrChannels = getNumberOfChannels(frameHeader);

        // Get regarding number of channels
        int size = (nbrChannels == 1) ? 17 : 32;

        // Get data from stream
        auto data = getDataFromStream(inputStream, size * 8);

        SideInformation sideInformation;
        unsigned int bitIndex = 0;

        // Get mainDataBegin
        sideInformation.mainDataBegin = getBitsAtIndex(data, bitIndex, 9);//TODO: verifier si c'est ok niveau endian et data sens, verifier si positif ou negatif

        // Get privateBits
        sideInformation.privateBits = getBitsAtIndex(data, bitIndex, (nbrChannels == 1) ? 5 : 3);

        // Get scaleFactor
        for (int c = 0; c < nbrChannels; ++c) {
            for (int i = 0; i < 4; ++i) {
                sideInformation.scaleFactorShare[c][i] = getBitsAtIndex(data, bitIndex, 1);
            }
        }

        // Get granules
        for (int i = 0; i < 2; ++i) {
            sideInformation.granules[i].resize(nbrChannels);

            for (int c = 0; c < nbrChannels; ++c) {
                sideInformation.granules[i][c] = readSideInformationGranule(data, bitIndex);
            }
        }

        return sideInformation;
    }

    SideInformationGranule Decoder::readSideInformationGranule(std::vector<uint8_t> const &data, unsigned int &bitIndex) const {
        SideInformationGranule sideInformationGranule;

        // Get par23Length
        sideInformationGranule.par23Length = getBitsAtIndex(data, bitIndex, 12);
        
        // Get bigValues
        sideInformationGranule.bigValues = getBitsAtIndex(data, bitIndex, 9);

        // Get globalGain
        sideInformationGranule.globalGain = getBitsAtIndex(data, bitIndex, 8);

        // Get scaleFacCompress
        auto scaleFactorCompressIndex = getBitsAtIndex(data, bitIndex, 4);

        sideInformationGranule.sLen1 = ScaleFactorCompress[scaleFactorCompressIndex][0];
        sideInformationGranule.sLen2 = ScaleFactorCompress[scaleFactorCompressIndex][1];

        // Get windowSwitchingFlag
        sideInformationGranule.windowSwitchingFlag = getBitsAtIndex(data, bitIndex, 1) != 0;

        // Check windowSwitchingFlag
        if (sideInformationGranule.windowSwitchingFlag == true) {
            // Get blockType
            sideInformationGranule.window.special.windowType = static_cast<WindowType>(getBitsAtIndex(data, bitIndex, 2));//TODO: aussi l'avoir dans windowSwitchingFlag == false avec comme valeur 0 pour eviter de checker a chaque fois windowSwitchingFlag quand on doit aussi regarder le type de window

            // Get mixedBlockFlag
            sideInformationGranule.window.special.mixedBlockFlag = getBitsAtIndex(data, bitIndex, 1) != 0;

            // Get tableSelect
            for (int i = 0; i < 2; ++i) {
                sideInformationGranule.tableSelect.push_back(getBitsAtIndex(data, bitIndex, 5));
            }

            // Get subblockGain
            for (int i = 0; i < 3; ++i) {
                sideInformationGranule.window.special.subblockGain[i] = getBitsAtIndex(data, bitIndex, 3);
            }

            // region0Count and region1Count are set to fixed values if short block
            sideInformationGranule.region0Count = ((sideInformationGranule.window.special.windowType == WindowType::ShortWindows3) && (sideInformationGranule.window.special.mixedBlockFlag == false)) ? 8 : 7;
            sideInformationGranule.region1Count = 36;
        } else {
            // Get tableSelect
            for (int i = 0; i < 3; ++i) {
                sideInformationGranule.tableSelect.push_back(getBitsAtIndex(data, bitIndex, 5));
            }

            // Get region0Count
            sideInformationGranule.region0Count = getBitsAtIndex(data, bitIndex, 4);

            // Get region1Count
            sideInformationGranule.region1Count = getBitsAtIndex(data, bitIndex, 3);
        }

        // Get preflag
        sideInformationGranule.preflag = getBitsAtIndex(data, bitIndex, 1);

        // Get scaleFacScale
        sideInformationGranule.scaleFacScale = ScaleFacScale[getBitsAtIndex(data, bitIndex, 1)];

        // Get count1TableIsB
        sideInformationGranule.count1TableIsB = getBitsAtIndex(data, bitIndex, 1) != 0;

        return sideInformationGranule; 
    }

    std::vector<uint8_t> Decoder::getDataFromStream(std::istream &inputStream, unsigned int sizeInBits) const {//TODO: si sizeInBits n'est pas multiple de 8 ???
        std::vector<uint8_t> data;

        // Get size in bytes
        auto sizeInBytes = (sizeInBits / 8) + (((sizeInBits % 8) != 0) ? 1 : 0);

        // Reserve size
        data.reserve(sizeInBytes);//TODO: a voir car ne modifie pas la taille du vector
        data.resize(sizeInBytes);

        // TODO: verifier taille flux

        // Read data
        inputStream.read(reinterpret_cast<char *>(data.data()), sizeInBytes);

        return data;
    }

    int Decoder::getScaleFactorShareGroupForScaleFactorBand(int scaleFactorBand) const {
        // Browse all scale factor share groups
        for (int scaleFactorShareGroupIndex = 1; scaleFactorShareGroupIndex < 5; ++scaleFactorShareGroupIndex) {
            // If scaleFactorBand is in current group
            if ((scaleFactorBand >= ScaleFactorShareGroupByScaleFactorBand[scaleFactorShareGroupIndex - 1]) && (scaleFactorBand < ScaleFactorShareGroupByScaleFactorBand[scaleFactorShareGroupIndex])) {
                // Return group index
                return scaleFactorShareGroupIndex;
            }
        }

        // Not found
        return -1;
    }

    ScaleFactors Decoder::readScaleFactors(std::vector<uint8_t> const &data, unsigned int &bitIndex, SideInformationGranule const &sideInformationGranule, bool const (&scaleFactorShare)[4], bool isFirstGranule) const {
        ScaleFactors scaleFactors;

        // If we have 3 short windows
        if ((sideInformationGranule.windowSwitchingFlag == true) && (sideInformationGranule.window.special.windowType == WindowType::ShortWindows3)) {
            // If we have mixed block
            if (sideInformationGranule.window.special.mixedBlockFlag == true) {
                // Browse scaleFactor bands 0 to 7 for long window
                for (int sfb = 0; sfb < 8; ++sfb) {
                    scaleFactors.longWindow[sfb] = getBitsAtIndex(data, bitIndex, sideInformationGranule.sLen1);
                }

                // Browse scaleFactor bands 3 to 11 for short window
                for (int sfb = 3; sfb < 12; ++sfb) {
                    // Browse windows
                    for (int w = 0; w < 3; ++w) {
                        scaleFactors.shortWindow[sfb][w] = getBitsAtIndex(data, bitIndex, (sfb < 6) ? sideInformationGranule.sLen1 : sideInformationGranule.sLen2);
                    }
                }                        
            } else {
                // Browse scaleFactor bands 0 to 11 for short window
                for (int sfb = 0; sfb < 12; ++sfb) {
                    // Browse windows
                    for (int w = 0; w < 3; ++w) {
                        scaleFactors.shortWindow[sfb][w] = getBitsAtIndex(data, bitIndex, (sfb < 6) ? sideInformationGranule.sLen1 : sideInformationGranule.sLen2);
                    }
                }
            }
        } else {
            // Browse scaleFactor bands 0 to 20 for long window
            for (int sfb = 0; sfb < 21; ++sfb) {
                // If we don't share or if we are in first granule
                if ((scaleFactorShare[getScaleFactorShareGroupForScaleFactorBand(sfb)] == false) || isFirstGranule) {
                    scaleFactors.longWindow[sfb] = getBitsAtIndex(data, bitIndex, (sfb < 11) ? sideInformationGranule.sLen1 : sideInformationGranule.sLen2);
                }
            }
        }

        return scaleFactors;
    }

    template <class TValueType>
    TValueType Decoder::decodeHuffmanCode(std::vector<uint8_t> const &data, unsigned int &bitIndex, std::unordered_map<unsigned int, TValueType> const &huffmanTable) const {
        // Calculate size
        int const huffmanCodeSizeInBits = data.size() * 8;

        // Browse huffman code
        for (unsigned int code = 1; bitIndex < huffmanCodeSizeInBits;) {
            code <<= 1;
            code |= getBitsAtIndex(data, bitIndex, 1);

            auto const &pair = huffmanTable.find(code);
            if (pair != std::end(huffmanTable)) {
                return pair->second;
            }
        }

        return {};
    }

    int Decoder::correctHuffmanDecodedValueSign(std::vector<uint8_t> const &data, unsigned int &bitIndex, int value) const {
        // Correct sign if necessary
        if (value != 0) {
            value *= (getBitsAtIndex(data, bitIndex, 1) == 0) ? 1 : -1;
        }

        return value;
    }

    int Decoder::correctHuffmanDecodedValue(std::vector<uint8_t> const &data, unsigned int &bitIndex, int value, unsigned int linbits) const {
        // Add linbits if necessary
        if ((value == 15) && (linbits > 0)) {
            value += getBitsAtIndex(data, bitIndex, linbits);
        }

        // Correct sign
        value = correctHuffmanDecodedValueSign(data, bitIndex, value);

        return value;
    }

    int Decoder::getRegionForFrequencyLine(SideInformationGranule const &sideInformationGranule, int frequencyLine, int samplingRateIndex) const {
        // If short windows
        if (sideInformationGranule.windowSwitchingFlag == true) {
            // Region 1 always starts at frequency line 36 and take all the remaining because there is no region 2
            return (frequencyLine < 36) ? 0 : 1;
        }

        // Get scaleFactorBands array for sampling rate
        auto const &scaleFactorBands = scaleFactorBandsLongBlockTable[samplingRateIndex];//TODO: si samplingRateIndex > 2 thrower une erreur !

        // If region 0
        if (frequencyLine <= scaleFactorBands[sideInformationGranule.region0Count]) {
            return 0;
        }

        // If region 1
        if (frequencyLine <= scaleFactorBands[sideInformationGranule.region0Count + 1 + sideInformationGranule.region1Count]) {
            return 1;
        }

        // Region 2
        return 2;
    }

    template <class TScaleFactorBandTable>
    int Decoder::getScaleFactorBandIndexForFrequencyLine(TScaleFactorBandTable const &scaleFactorBands, int frequencyLine) const {
        // Browse all scale factor bands
        for (int scaleFactorBandIndex = 0; scaleFactorBandIndex < scaleFactorBands.size(); ++scaleFactorBandIndex) {
            // If frequency line is in current scale factor band
            if (frequencyLine <= scaleFactorBands[scaleFactorBandIndex]) {
                // Return scaleFactorBandIndex
                return scaleFactorBandIndex;
            }
        }

        // Not found
        return -1;//TODO: a voir si on depasse
    }

    std::vector<int> Decoder::readHuffmanCode(std::vector<uint8_t> const &data, unsigned int &bitIndex, SideInformationGranule const &sideInformationGranule, int samplingRateIndex) const {
        std::vector<int> huffmanDecoded(576);//TODO: peut etre par apres l'avoir en membre pour eviter l'allocation a chaque frame/granule/channel mais faire attention si plusieurs frames doivent etre gardées en mémoire

        // Browse all frequency lines
        for (int frequencyLineIndex = 0; frequencyLineIndex < 576; ++frequencyLineIndex) {
            // Start with bigValues
            if (frequencyLineIndex < (sideInformationGranule.bigValues * 2)) {
                // Get current region
                int currentRegion = getRegionForFrequencyLine(sideInformationGranule, frequencyLineIndex, samplingRateIndex);

                // Get huffman table index
                auto huffmanTableIndex = sideInformationGranule.tableSelect[currentRegion];

                QuantizedValuePair decodedValue = { 0, 0 };

                // If huffman table is zero, don't read in stream but set all the region to 0
                if (huffmanTableIndex > 0) {//TODO: a voir si ainsi
                    // Get huffman table
                    auto const &huffmanData = bigValuesData[huffmanTableIndex];

                    // Decode data
                    decodedValue = decodeHuffmanCode(data, bitIndex, huffmanData.table);

                    // Correct data
                    decodedValue.x = correctHuffmanDecodedValue(data, bitIndex, decodedValue.x, huffmanData.linbits);
                    decodedValue.y = correctHuffmanDecodedValue(data, bitIndex, decodedValue.y, huffmanData.linbits);
                }

                // Add to huffmanDecoded
                huffmanDecoded[frequencyLineIndex] = decodedValue.x;
                ++frequencyLineIndex;
                huffmanDecoded[frequencyLineIndex] = decodedValue.y;
            }
            // Then we have Count1 values
            else if (bitIndex < sideInformationGranule.par23Length) {
                QuantizedValueQuadruple decodedValue;

                // Decode value if table is A
                if (sideInformationGranule.count1TableIsB == false) {
                    // Decode data
                    decodedValue = decodeHuffmanCode(data, bitIndex, count1TableA);
                }
                // Simply get 4 bits and invert them if table is B
                else {
                    // Get data
                    decodedValue.v = getBitsAtIndex(data, bitIndex, 1) ^ 0x1;
                    decodedValue.w = getBitsAtIndex(data, bitIndex, 1) ^ 0x1;
                    decodedValue.x = getBitsAtIndex(data, bitIndex, 1) ^ 0x1;
                    decodedValue.y = getBitsAtIndex(data, bitIndex, 1) ^ 0x1;
                }

                // Correct data
                decodedValue.v = correctHuffmanDecodedValueSign(data, bitIndex, decodedValue.v);
                decodedValue.w = correctHuffmanDecodedValueSign(data, bitIndex, decodedValue.w);
                decodedValue.x = correctHuffmanDecodedValueSign(data, bitIndex, decodedValue.x);
                decodedValue.y = correctHuffmanDecodedValueSign(data, bitIndex, decodedValue.y);

                // Add to huffmanDecoded
                huffmanDecoded[frequencyLineIndex] = decodedValue.v;
                ++frequencyLineIndex;
                huffmanDecoded[frequencyLineIndex] = decodedValue.w;
                ++frequencyLineIndex;
                huffmanDecoded[frequencyLineIndex] = decodedValue.x;
                ++frequencyLineIndex;
                huffmanDecoded[frequencyLineIndex] = decodedValue.y;
            }
            // Then we have Rzero values
            else {
                huffmanDecoded[frequencyLineIndex] = 0;
            }
        }

        return huffmanDecoded;
    }

    std::vector<int> Decoder::requantize(std::vector<int> const &values, ScaleFactors const &scaleFactors, SideInformationGranule const &sideInformationGranule, int samplingRateIndex) const {
        std::vector<int> requantizedValues(576);

        int currentWindowIndex = 0;

        for (int frequencyLineIndex = 0; frequencyLineIndex < 576; ++frequencyLineIndex) {//TODO: toutes les references a 576 doivent etre retirées et remplacées par un const
            int gainCorrection;
            int scaleFactor;

            // Short block
            if ((sideInformationGranule.windowSwitchingFlag == true) && (sideInformationGranule.window.special.windowType == WindowType::ShortWindows3) && ((sideInformationGranule.window.special.mixedBlockFlag == false) || (frequencyLineIndex >= 36))) {//TODO: a voir
                // Gain correction by subblock
                gainCorrection = 8 * sideInformationGranule.window.special.subblockGain[currentWindowIndex];

                // Get scale factor
                scaleFactor = scaleFactors.shortWindow[getScaleFactorBandIndexForFrequencyLine(scaleFactorBandsShortBlockTable[samplingRateIndex], frequencyLineIndex / 3)][currentWindowIndex];

                // Update currentWindowIndex
                currentWindowIndex = (currentWindowIndex + 1) % 3;//TODO: pas bon, a changer !
            }
            // Long block
            else {
                // No gain correction
                gainCorrection = 0;

                // Get scale factor
                auto scaleFactorBandIndex = getScaleFactorBandIndexForFrequencyLine(scaleFactorBandsLongBlockTable[samplingRateIndex], frequencyLineIndex);
                scaleFactor = scaleFactors.longWindow[scaleFactorBandIndex] + (sideInformationGranule.preflag * PretabByScaleFactorBand[scaleFactorBandIndex]);
            }

            // Get current value sign
            int currentValueSign = (values[frequencyLineIndex] >= 0) ? 1 : -1;

            // Calculate power gain
            float powerGain = std::pow(2.0f, (sideInformationGranule.globalGain - 210 - gainCorrection) / 4.0f);//TODO: voir si 210 sera dans un const

            // Calculate power scale factor
            float powerScaleFactor = std::pow(2.0f, -(sideInformationGranule.scaleFacScale * scaleFactor));

            // Requantize current value
            requantizedValues[frequencyLineIndex] = currentValueSign * std::pow(std::abs(values[frequencyLineIndex]), 4.0f / 3.0f) * powerGain * powerScaleFactor;
        }

        //TODO: voir pour les int avec float, si besoin de conversion !

        return requantizedValues;
    }

    ScaleFactors Decoder::readMainData(std::istream &inputStream, FrameHeader const &frameHeader, SideInformation const &sideInformation) {
        // Get number of channels
        int nbrChannels = getNumberOfChannels(frameHeader);

        // Browse granules
        for (int g = 0; g < 2; ++g) {
            // Browse channels;
            for (int c = 0; c < nbrChannels; ++c) {
                // Get current sideInformationGranule
                auto const &sideInformationGranule = sideInformation.granules[g][c];

                // TODO: voir pour le bit reservoir

                // Get data
                auto data = getDataFromStream(inputStream, sideInformationGranule.par23Length);
                unsigned int bitIndex = 0;

                // Read scale factors
                auto const &scaleFactorShare = sideInformation.scaleFactorShare[c];
                auto scaleFactors = readScaleFactors(data, bitIndex, sideInformationGranule, scaleFactorShare, g == 0);

                // Read huffman code
                auto huffmanCode = readHuffmanCode(data, bitIndex, sideInformationGranule, frameHeader.samplingRateIndex);

                // Requantize data
                requantize(huffmanCode, scaleFactors, sideInformationGranule, frameHeader.samplingRateIndex);
            }
        }

        // TODO: lire les ancillary data et le reste des données doit aller dans le bit reservoir

        return { };//TODO: a changer
    }

    int Decoder::getFrameSize(FrameHeader const &frameHeader) const {
        if (SamplingRateTable[frameHeader.samplingRateIndex] == 0) {
            return 0;
        }

        return ((144000 * frameHeader.bitrate) / SamplingRateTable[frameHeader.samplingRateIndex]) + ((frameHeader.isPadded) ? 1 : 0);
    }

}
