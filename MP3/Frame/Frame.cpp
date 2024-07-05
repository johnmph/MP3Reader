
#include <cmath>
#include "Frame.hpp"
#include "../Helper.hpp"
#include "Data/Frame.hpp"
#include "Data/HuffmanTables.hpp"
#include "Data/ScaleFactorBands.hpp"
#include "Data/SynthesisFilterBank.hpp"
#include "Data/SynthesisWindow.hpp"
static bool _isInitialized = false;
//TODO: bien verifier pour les conditions et le code executé pour les types de block, si mixed ou pas, ...
namespace MP3::Frame {
std::vector<std::array<float, 1024>> Frame::_shiftedAndMatrixedSubbandsValues;
    Frame::Frame(Header const &header, SideInformation const &sideInformation, unsigned int const ancillaryDataSizeInBits, std::vector<uint8_t> const &data) : _header(header), _sideInformation(sideInformation), _data(data), _dataBitIndex(0) {
        // Resize frequencyLineValues vector (always 2 granules but variable number of channels)
        _frequencyLineValues.resize(2 * _header.getNumberOfChannels());

        // Resize subbandsValues vector (always 2 granules but variable number of channels)
        _subbandsValues.resize(2 * _header.getNumberOfChannels());

        // Resize pcmValues vector (always 2 granules but variable number of channels)
        _pcmValues.resize(2 * _header.getNumberOfChannels());

        // Resize startingRzeroFrequencyLineIndexes vector (always 2 granules but variable number of channels)
        _startingRzeroFrequencyLineIndexes.resize(2 * _header.getNumberOfChannels());
if (_isInitialized == false) {
_shiftedAndMatrixedSubbandsValues.resize(_header.getNumberOfChannels());
for (unsigned int i = 0; i < _header.getNumberOfChannels(); ++i) {
            std::fill(std::begin(_shiftedAndMatrixedSubbandsValues[i]), std::end(_shiftedAndMatrixedSubbandsValues[i]), 0.0f);//TODO: doit etre initialisé a 0, voir si pas deja via le resize dans le constructor
}
_isInitialized = true;
}
        // Extract main data
        extractMainData();
    }

    std::vector<uint8_t> const &Frame::getAncillaryBits() const {
        //TODO: terminer
        return {};
    }

    std::array<float, 576> const &Frame::getPCMSamples(unsigned int const granuleIndex, unsigned int const channelIndex) const {
        return _pcmValues[(granuleIndex * _header.getNumberOfChannels()) + channelIndex];
    }

    void Frame::extractMainData() {
        // Browse granules
        for (unsigned int granuleIndex = 0; granuleIndex < 2; ++granuleIndex) {
            // Browse channels
            for (unsigned int channelIndex = 0; channelIndex < _header.getNumberOfChannels(); ++channelIndex) {
                // Save dataBitIndex before start extracting granule data
                auto const granuleStartDataBitIndex = _dataBitIndex;

                // Extract scale factors
                extractScaleFactors(granuleIndex, channelIndex);

                // Extract frequency line values
                extractFrequencyLineValues(granuleIndex, channelIndex, granuleStartDataBitIndex);

                // Requantize frequency line values
                requantizeFrequencyLineValues(granuleIndex, channelIndex);

                // Reorder short windows
                reorderShortWindows(granuleIndex, channelIndex);
            }

            // Process stereo (need to be done when all channels are decoded)
            processStereo(granuleIndex);
        }

        // What is following can't be done before finishing processStereo, it's why we separated it to another loop
        // Browse granules
        for (unsigned int granuleIndex = 0; granuleIndex < 2; ++granuleIndex) {
            // Browse channels
            for (unsigned int channelIndex = 0; channelIndex < _header.getNumberOfChannels(); ++channelIndex) {
                // Synthesis FilterBank
                synthesisFilterBank(granuleIndex, channelIndex);
            }
        }
    }

    void Frame::extractScaleFactors(unsigned int const granuleIndex, unsigned int const channelIndex) {
        // Get current granule
        auto const &currentGranule = _sideInformation.getGranule(granuleIndex, channelIndex);

        // If we have 3 short windows
        if (currentGranule.blockType == BlockType::ShortWindows3) {
            // If we have mixed block
            if (std::get<SideInformationGranuleSpecialWindow>(currentGranule.window).mixedBlockFlag == true) {
                // Browse scaleFactor bands 0 to 7 for long window
                for (unsigned int scaleFactorBandIndex = 0; scaleFactorBandIndex < 8; ++scaleFactorBandIndex) {
                    _scaleFactors.longWindow[scaleFactorBandIndex] = Helper::getBitsAtIndex(_data, _dataBitIndex, currentGranule.sLen1);
                }

                // Browse scaleFactor bands 3 to 11 for short window
                for (unsigned int scaleFactorBandIndex = 3; scaleFactorBandIndex < 12; ++scaleFactorBandIndex) {
                    // Browse windows
                    for (unsigned int windowIndex = 0; windowIndex < 3; ++windowIndex) {
                        _scaleFactors.shortWindow[scaleFactorBandIndex][windowIndex] = Helper::getBitsAtIndex(_data, _dataBitIndex, (scaleFactorBandIndex < 6) ? currentGranule.sLen1 : currentGranule.sLen2);
                    }
                }                        
            } else {
                // Browse scaleFactor bands 0 to 11 for short window
                for (unsigned int scaleFactorBandIndex = 0; scaleFactorBandIndex < 12; ++scaleFactorBandIndex) {
                    // Browse windows
                    for (unsigned int windowIndex = 0; windowIndex < 3; ++windowIndex) {
                        _scaleFactors.shortWindow[scaleFactorBandIndex][windowIndex] = Helper::getBitsAtIndex(_data, _dataBitIndex, (scaleFactorBandIndex < 6) ? currentGranule.sLen1 : currentGranule.sLen2);
                    }
                }
            }
        } else {
            // Browse scaleFactor bands 0 to 20 for long window
            for (unsigned int scaleFactorBandIndex = 0; scaleFactorBandIndex < 21; ++scaleFactorBandIndex) {
                // If we don't share or if we are in first granule
                if ((_sideInformation.getScaleFactorShare(channelIndex, getScaleFactorShareGroupForScaleFactorBand(scaleFactorBandIndex)) == false) || (granuleIndex == 0)) {
                    _scaleFactors.longWindow[scaleFactorBandIndex] = Helper::getBitsAtIndex(_data, _dataBitIndex, (scaleFactorBandIndex < 11) ? currentGranule.sLen1 : currentGranule.sLen2);
                }
            }
        }
    }

    void Frame::extractFrequencyLineValues(unsigned int const granuleIndex, unsigned int const channelIndex, unsigned int const granuleStartDataBitIndex) {
        // Get current granule
        auto const &currentGranule = _sideInformation.getGranule(granuleIndex, channelIndex);

        // Get current frequency line values vector
        auto &currentFrequencyLineValues = _frequencyLineValues[(granuleIndex * _header.getNumberOfChannels()) + channelIndex];

        // Get current starting Rzero frequency line index
        auto &currentStartingRzeroFrequencyLineIndex = _startingRzeroFrequencyLineIndexes[(granuleIndex * _header.getNumberOfChannels()) + channelIndex];
        currentStartingRzeroFrequencyLineIndex = 0;

        // Browse all frequency lines
        for (unsigned int frequencyLineIndex = 0; frequencyLineIndex < 576; ++frequencyLineIndex) {//TODO: 576 dans une const
            // Start with bigValues
            if (frequencyLineIndex < (currentGranule.bigValues * 2)) {
                // Get current region
                auto const currentRegion = getBigValuesRegionForFrequencyLineIndex(currentGranule, frequencyLineIndex);

                // Get huffman table index
                auto const huffmanTableIndex = currentGranule.tableSelect[currentRegion];

                Data::QuantizedValuePair decodedValue = { 0, 0 };

                // If huffman table is zero, don't read in stream but set all the region to 0
                if (huffmanTableIndex > 0) {//TODO: a voir si ainsi
                    // Get huffman table
                    auto const &huffmanData = Data::bigValuesData[huffmanTableIndex];

                    // Decode data
                    decodedValue = decodeHuffmanCode(currentGranule.par23Length, granuleStartDataBitIndex, huffmanData.table);

                    // Correct data
                    decodedValue.x = huffmanCodeApplyLinbitsAndSign(decodedValue.x, huffmanData.linbits);
                    decodedValue.y = huffmanCodeApplyLinbitsAndSign(decodedValue.y, huffmanData.linbits);
                }

                // Add to currentFrequencyLineValues
if (frequencyLineIndex > 574) {
break;
}
                currentFrequencyLineValues[frequencyLineIndex] = decodedValue.x;
                ++frequencyLineIndex;
                currentFrequencyLineValues[frequencyLineIndex] = decodedValue.y;
            }
            // Then we have Count1 values
            else if ((_dataBitIndex - granuleStartDataBitIndex) < currentGranule.par23Length) {
if (frequencyLineIndex == 460) {
int pp = 0;
}
                Data::QuantizedValueQuadruple decodedValue;

                // Decode value if table is A
                if (currentGranule.isCount1TableB == false) {
                    // Decode data
                    decodedValue = decodeHuffmanCode(currentGranule.par23Length, granuleStartDataBitIndex, Data::count1TableA);
                }
                // Simply get 4 bits and invert them if table is B
                else {
                    // Get data
                    decodedValue.v = Helper::getBitsAtIndex(_data, _dataBitIndex, 1) ^ 0x1;
                    decodedValue.w = Helper::getBitsAtIndex(_data, _dataBitIndex, 1) ^ 0x1;
                    decodedValue.x = Helper::getBitsAtIndex(_data, _dataBitIndex, 1) ^ 0x1;
                    decodedValue.y = Helper::getBitsAtIndex(_data, _dataBitIndex, 1) ^ 0x1;
                }

                // Correct data
                decodedValue.v = huffmanCodeApplySign(decodedValue.v);
                decodedValue.w = huffmanCodeApplySign(decodedValue.w);
                decodedValue.x = huffmanCodeApplySign(decodedValue.x);
                decodedValue.y = huffmanCodeApplySign(decodedValue.y);

                // Add to currentFrequencyLineValues
if (frequencyLineIndex > 572) {
break;
}
                currentFrequencyLineValues[frequencyLineIndex] = decodedValue.v;
                ++frequencyLineIndex;
                currentFrequencyLineValues[frequencyLineIndex] = decodedValue.w;
                ++frequencyLineIndex;
                currentFrequencyLineValues[frequencyLineIndex] = decodedValue.x;
                ++frequencyLineIndex;
                currentFrequencyLineValues[frequencyLineIndex] = decodedValue.y;
            }
            // Then we have Rzero values
            else {
                // Get starting frequency line index for Rzero if not already found
                if (currentStartingRzeroFrequencyLineIndex == 0) {
                    currentStartingRzeroFrequencyLineIndex = frequencyLineIndex;
                }

                // Add 0 to currentFrequencyLineValues
                currentFrequencyLineValues[frequencyLineIndex] = 0;
            }
        }

// TODO: dans le cas ou on dépasse 576, il n'y a pas de rzero mais voir si c normal de depasser
if (currentStartingRzeroFrequencyLineIndex == 0) {
    currentStartingRzeroFrequencyLineIndex = 576;
}

    }

    void Frame::requantizeFrequencyLineValues(unsigned int const granuleIndex, unsigned int const channelIndex) {
        // Get current granule
        auto const &currentGranule = _sideInformation.getGranule(granuleIndex, channelIndex);

        // Get current frequency line values vector
        auto &currentFrequencyLineValues = _frequencyLineValues[(granuleIndex * _header.getNumberOfChannels()) + channelIndex];

        // Browse frequency lines
        for (unsigned int frequencyLineIndex = 0; frequencyLineIndex < 576; ++frequencyLineIndex) {//TODO: toutes les references a 576 doivent etre retirées et remplacées par un const
            // Get gainCorrection
            unsigned int const gainCorrection = (isFrequencyLineIndexInShortBlock(currentGranule, frequencyLineIndex) == true) ? (8 * std::get<SideInformationGranuleSpecialWindow>(currentGranule.window).subblockGain[getCurrentWindowIndexForFrequencyLineIndex(frequencyLineIndex)]) : 0;

            // Get scaleFactor
            unsigned int const scaleFactor = getScaleFactorForFrequencyLineIndex(currentGranule, frequencyLineIndex);

            // Get current value sign
            int const currentValueSign = (currentFrequencyLineValues[frequencyLineIndex] >= 0) ? 1 : -1;

            // Calculate power gain
            float const powerGain = std::pow(2.0f, static_cast<int>(currentGranule.globalGain - (210 + gainCorrection)) / 4.0f);//TODO: voir si 210 sera dans un const

            // Calculate power scale factor
            float const powerScaleFactor = std::pow(2.0f, -(currentGranule.scaleFacScale * scaleFactor));

            // Requantize current value
            currentFrequencyLineValues[frequencyLineIndex] = currentValueSign * std::pow(std::abs(currentFrequencyLineValues[frequencyLineIndex]), 4.0f / 3.0f) * powerGain * powerScaleFactor;
        }

        //TODO: voir pour les int avec float, si besoin de conversion !
    }

    void Frame::reorderShortWindows(unsigned int const granuleIndex, unsigned int const channelIndex) {//TODO: voir si le reorder est correct ainsi
    // TODO: a la place de calculer les indexs de reorder ici, avoir des tableaux constants calculés au démarrage avec une methode constexpr ! et ici on utilisera les indexs du bon tableau reorder pour reorder les données
        // Get current granule
        auto const &currentGranule = _sideInformation.getGranule(granuleIndex, channelIndex);

        // Don't reorder other than short block
        if (currentGranule.blockType != BlockType::ShortWindows3) {
            return;
        }

        // Get current frequency line values vector
        auto &currentFrequencyLineValues = _frequencyLineValues[(granuleIndex * _header.getNumberOfChannels()) + channelIndex];
        auto savedCurrentFrequencyLineValues = currentFrequencyLineValues;
        
        // Browse frequency lines
        unsigned int startFrequencyLineForCurrentScaleFactorBand = 0;
        unsigned int reorderedIndex = 0;

        for (unsigned int frequencyLineIndex = ((std::get<SideInformationGranuleSpecialWindow>(currentGranule.window).mixedBlockFlag == true) ? 36 : 0); frequencyLineIndex < 576; ++frequencyLineIndex) {//TODO: toutes les references a 576 doivent etre retirées et remplacées par un const et 36 aussi
            // Get current scaleFactor band index
            auto const currentScaleFactorBandIndex = getScaleFactorBandIndexForFrequencyLineIndex(Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()], frequencyLineIndex / 3);

            // Get current scaleFactor band max frequency line
            int const currentScaleFactorBandMaxFrequencyLine = Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()][currentScaleFactorBandIndex];

            // Reorder
            currentFrequencyLineValues[frequencyLineIndex] = savedCurrentFrequencyLineValues[startFrequencyLineForCurrentScaleFactorBand + reorderedIndex];

            // Update reorderedIndex
            reorderedIndex += currentScaleFactorBandMaxFrequencyLine - ((currentScaleFactorBandIndex > 0) ? Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()][currentScaleFactorBandIndex - 1] : -1);

            if (reorderedIndex >= ((currentScaleFactorBandMaxFrequencyLine + 1) * 3)) {
                reorderedIndex -= ((currentScaleFactorBandMaxFrequencyLine + 1) * 3) - 1;

                // The current scale factor band index is over, go to next
                if (reorderedIndex == (currentScaleFactorBandMaxFrequencyLine + 1)) {
                    reorderedIndex = 0;
                    startFrequencyLineForCurrentScaleFactorBand = frequencyLineIndex + 1;
                }
            }
        }
    }

    void Frame::processStereo(unsigned int const granuleIndex) {
        // Exit if not joint stereo
        if (_header.getChannelMode() != ChannelMode::JointStereo) {
            return;
        }

        // Process MSStereo if necessary
        if (_header.isMSStereo() == true) {
            processMSStereo(granuleIndex);
        }

        // Process IntensityStereo if necessary
        if (_header.isIntensityStereo() == true) {
            processIntensityStereo(granuleIndex);
        }
    }

    void Frame::synthesisFilterBank(unsigned int const granuleIndex, unsigned int const channelIndex) {
        // Apply alias reduction
        applyAliasReduction(granuleIndex, channelIndex);

        //TODO: voir si ok avec mixedBlockFlag pour IMDCT et overlapping !!!
        // Get current granule
        auto const &currentGranule = _sideInformation.getGranule(granuleIndex, channelIndex);

        // Get current frequency line values vector
        auto const &currentFrequencyLineValues = _frequencyLineValues[(granuleIndex * _header.getNumberOfChannels()) + channelIndex];

        // Get current subbands values vector
        auto &currentSubbandsValues = _subbandsValues[(granuleIndex * _header.getNumberOfChannels()) + channelIndex];

        // Initialize currentSubbandsValues to 0
        std::fill(std::begin(currentSubbandsValues), std::end(currentSubbandsValues), 0.0f);//TODO: doit etre initialisé a 0, voir si pas deja via le resize dans le constructor, si non voir si pas plutot le mettre dans le constructor

        // Browse subbands
        for (unsigned int subbandIndex = 0; subbandIndex < 32; ++subbandIndex) {//TODO: a repeter 32x pour toutes les subbands ? si oui faire attention aux indexs
            // Create temporary array to keep current subband values
            std::array<float, 36> subbandValues = {};

            // Apply IMDCT
            applyIMDCT(currentGranule, currentFrequencyLineValues, subbandValues, subbandIndex);

            // Apply windowing
            applyWindowing(currentGranule, subbandValues, subbandIndex);

            // Apply overlapping
            applyOverlapping(subbandValues, currentSubbandsValues, subbandIndex);

            // Apply compensation for frequency inversion
            applyCompensationForFrequencyInversion(currentSubbandsValues, subbandIndex);
        }

        // Apply polyphase filterbank
        applyPolyphaseFilterBank(granuleIndex, channelIndex);
    }

    unsigned int Frame::getScaleFactorShareGroupForScaleFactorBand(unsigned int const scaleFactorBand) const {
        // Browse all scale factor share groups
        for (unsigned int scaleFactorShareGroupIndex = 1; scaleFactorShareGroupIndex < 5; ++scaleFactorShareGroupIndex) {
            // If scaleFactorBand is in current group
            if ((scaleFactorBand >= Data::scaleFactorBandsPerScaleFactorShareGroup[scaleFactorShareGroupIndex - 1]) && (scaleFactorBand < Data::scaleFactorBandsPerScaleFactorShareGroup[scaleFactorShareGroupIndex])) {
                // Return group index
                return scaleFactorShareGroupIndex;
            }
        }

        // Not found
        return -1;  // TODO: attention on retourne -1 en unsigned int
    }

    unsigned int Frame::getBigValuesRegionForFrequencyLineIndex(SideInformationGranule const &sideInformationGranule, unsigned int const frequencyLineIndex) const {
        // If short windows
        if (sideInformationGranule.windowSwitchingFlag == true) {
            // Region 1 always starts at frequency line 36 and take all the remaining because there is no region 2
            return (frequencyLineIndex < 36) ? 0 : 1;
        }

        // Get scaleFactorBands array for sampling rate
        auto const &scaleFactorBands = Data::frequencyLinesPerScaleFactorBandLongBlock[_header.getSamplingRateIndex()];//TODO: si samplingRateIndex > 2 thrower une erreur !

        // If region 0
        if (frequencyLineIndex <= scaleFactorBands[sideInformationGranule.region0Count]) {
            return 0;
        }

        // If region 1
        if (frequencyLineIndex <= scaleFactorBands[sideInformationGranule.region0Count + 1 + sideInformationGranule.region1Count]) {
            return 1;
        }

        // Region 2
        return 2;
    }

    template <class TValueType>
    TValueType Frame::decodeHuffmanCode(unsigned int const granulePart23LengthInBits, unsigned int const granuleStartDataBitIndex, std::unordered_map<unsigned int, TValueType> const &huffmanTable) {
        // Browse huffman code
        for (unsigned int code = 1; (_dataBitIndex - granuleStartDataBitIndex) < granulePart23LengthInBits;) {
            code <<= 1;
            code |= Helper::getBitsAtIndex(_data, _dataBitIndex, 1);

            auto const &pair = huffmanTable.find(code);
            if (pair != std::end(huffmanTable)) {
                return pair->second;
            }
        }

        return {};
    }

    int Frame::huffmanCodeApplyLinbitsAndSign(int value, unsigned int const linbits) {
        //TODO: thrower si on depasse la taille de la granule
        // Add linbits if necessary
        if ((value == 15) && (linbits > 0)) {
            value += Helper::getBitsAtIndex(_data, _dataBitIndex, linbits);
        }

        // Correct sign
        value = huffmanCodeApplySign(value);

        return value;
    }

    int Frame::huffmanCodeApplySign(int value) {
        //TODO: thrower si on depasse la taille de la granule
        // Correct sign if necessary
        if (value != 0) {
            value *= (Helper::getBitsAtIndex(_data, _dataBitIndex, 1) == 0) ? 1 : -1;
        }

        return value;
    }

    template <class TScaleFactorBandTable>
    unsigned int Frame::getScaleFactorBandIndexForFrequencyLineIndex(TScaleFactorBandTable const &scaleFactorBands, unsigned int const frequencyLineIndex) const {
        // Browse all scale factor bands
        for (unsigned int scaleFactorBandIndex = 0; scaleFactorBandIndex < scaleFactorBands.size(); ++scaleFactorBandIndex) {
            // If frequency line index is in current scale factor band
            if (frequencyLineIndex <= scaleFactorBands[scaleFactorBandIndex]) {
                // Return scaleFactorBandIndex
                return scaleFactorBandIndex;
            }
        }

        // Not found
        return -1;//TODO: a voir si on depasse si on veut garder -1 alors std::numeric_limits<unsigned int>::max()
    }

    unsigned int Frame::getCurrentWindowIndexForFrequencyLineIndex(unsigned int const frequencyLineIndex) const {
        // Get scaleFactor band index
        auto const scaleFactorBandIndex = getScaleFactorBandIndexForFrequencyLineIndex(Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()], frequencyLineIndex / 3);

        // Get currentWindowIndex
        int const currentScaleFactorBandMaxFrequencyLine = Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()][scaleFactorBandIndex];
        int const currentScaleFactorBandLastMaxFrequencyLine = (scaleFactorBandIndex > 0) ? Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()][scaleFactorBandIndex - 1] : -1;

        return ((frequencyLineIndex / 3) - (currentScaleFactorBandLastMaxFrequencyLine + 1)) / (currentScaleFactorBandMaxFrequencyLine - currentScaleFactorBandLastMaxFrequencyLine);
    }

    unsigned int Frame::getScaleFactorForFrequencyLineIndex(SideInformationGranule const &sideInformationGranule, unsigned int const frequencyLineIndex) const {
        // Short block
        if (isFrequencyLineIndexInShortBlock(sideInformationGranule, frequencyLineIndex)) {
            // Get scaleFactor band index
            auto const scaleFactorBandIndex = getScaleFactorBandIndexForFrequencyLineIndex(Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()], frequencyLineIndex / 3);

            // Get scale factor
            return (scaleFactorBandIndex != -1) ? _scaleFactors.shortWindow[scaleFactorBandIndex][getCurrentWindowIndexForFrequencyLineIndex(frequencyLineIndex)] : 0;//TODO: a voir pour -1
        }

        // Long block
        // Get scale factor
        auto const scaleFactorBandIndex = getScaleFactorBandIndexForFrequencyLineIndex(Data::frequencyLinesPerScaleFactorBandLongBlock[_header.getSamplingRateIndex()], frequencyLineIndex);
        return (scaleFactorBandIndex != -1) ? _scaleFactors.longWindow[scaleFactorBandIndex] + (sideInformationGranule.preflag * Data::pretabByScaleFactorBand[scaleFactorBandIndex]) : 0; // TODO: a voir pour -1
    }

    bool Frame::isFrequencyLineIndexInShortBlock(SideInformationGranule const &sideInformationGranule, unsigned int const frequencyLineIndex) const {
        return (sideInformationGranule.blockType == BlockType::ShortWindows3) && ((std::get<SideInformationGranuleSpecialWindow>(sideInformationGranule.window).mixedBlockFlag == false) || (frequencyLineIndex >= 36));
    }

    void Frame::processMSStereo(unsigned int const granuleIndex) {
        // TODO: tester si les blocks des 2 canaux sont de memes types sinon throw exception ?

        // Get intensityStereoBound
        auto const intensityStereoBound = (_header.isIntensityStereo() == true) ? getIntensityStereoBound(granuleIndex) : 576;

        // Get current frequency line values vector
        auto &currentFrequencyLineValuesLeft = _frequencyLineValues[(granuleIndex * _header.getNumberOfChannels()) + 0];
        auto &currentFrequencyLineValuesRight = _frequencyLineValues[(granuleIndex * _header.getNumberOfChannels()) + 1];

        // Process
        for (unsigned int frequencyLineIndex = 0; frequencyLineIndex < intensityStereoBound; ++frequencyLineIndex) {
            auto const leftValue = (currentFrequencyLineValuesLeft[frequencyLineIndex] + currentFrequencyLineValuesRight[frequencyLineIndex]) / std::sqrt(2.0f);
            auto const rightValue = (currentFrequencyLineValuesLeft[frequencyLineIndex] - currentFrequencyLineValuesRight[frequencyLineIndex]) / std::sqrt(2.0f);

            currentFrequencyLineValuesLeft[frequencyLineIndex] = leftValue;
            currentFrequencyLineValuesRight[frequencyLineIndex] = rightValue;
        }
    }

    void Frame::processIntensityStereo(unsigned int const granuleIndex) {//TODO: a tester avec un autre fichier car il n'utilise pas l'intensity stereo
        // Get current granule right channel
        auto const &currentGranuleRight = _sideInformation.getGranule(granuleIndex, 1);

        // Get current frequency line values vector
        auto &currentFrequencyLineValuesLeft = _frequencyLineValues[(granuleIndex * _header.getNumberOfChannels()) + 0];
        auto &currentFrequencyLineValuesRight = _frequencyLineValues[(granuleIndex * _header.getNumberOfChannels()) + 1];

        // Process
        for (unsigned int frequencyLineIndex = getIntensityStereoBound(granuleIndex); frequencyLineIndex < 576; ++frequencyLineIndex) {
            // Get isPosSB which is scaleFactor of the current scaleFactorBand group
            auto const isPosSB = getScaleFactorForFrequencyLineIndex(currentGranuleRight, frequencyLineIndex);    // TODO: voir si c'est bon

            // Don't process if illegal
            if (isPosSB == 7) {
                continue;
            }

            // Calculate isRatio
            auto const isRatio = std::tan(isPosSB * (M_PI / 12.0f));//TODO: a voir pour la constante et les types

            // Correct values
            auto const leftValue = currentFrequencyLineValuesLeft[frequencyLineIndex] * (isRatio / (1.0f + isRatio));
            auto const rightValue = currentFrequencyLineValuesLeft[frequencyLineIndex] * (1.0f / (1.0f + isRatio));

            currentFrequencyLineValuesLeft[frequencyLineIndex] = leftValue;
            currentFrequencyLineValuesRight[frequencyLineIndex] = rightValue;
        }
    }

    unsigned int Frame::getIntensityStereoBound(unsigned int const granuleIndex) const {
        // Get current granule right channel
        auto const &currentGranuleRight = _sideInformation.getGranule(granuleIndex, 1);

        // Get current starting Rzero frequency line index (right channel)
        auto const currentStartingRzeroFrequencyLineIndex = _startingRzeroFrequencyLineIndexes[(granuleIndex * _header.getNumberOfChannels()) + 1] - 1;

        // Short block
        if (isFrequencyLineIndexInShortBlock(currentGranuleRight, currentStartingRzeroFrequencyLineIndex) == true) {//TODO: a voir
            // Get current scaleFactor band index
            auto const currentScaleFactorBandIndex = getScaleFactorBandIndexForFrequencyLineIndex(Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()], currentStartingRzeroFrequencyLineIndex / 3);

            // Upper bound is last frequency line index for this scaleFactorBand group
            return Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()][currentScaleFactorBandIndex] + 1;
        }

        // Long block
        // Get current scaleFactor band index
        auto const currentScaleFactorBandIndex = getScaleFactorBandIndexForFrequencyLineIndex(Data::frequencyLinesPerScaleFactorBandLongBlock[_header.getSamplingRateIndex()], currentStartingRzeroFrequencyLineIndex);

        // Upper bound is last frequency line index for this scaleFactorBand group
        return Data::frequencyLinesPerScaleFactorBandLongBlock[_header.getSamplingRateIndex()][currentScaleFactorBandIndex] + 1;
    }

    void Frame::applyAliasReduction(unsigned int const granuleIndex, unsigned int const channelIndex) {
        // Get current granule
        auto const &currentGranule = _sideInformation.getGranule(granuleIndex, channelIndex);

        // Don't reorder other than long block
        if ((currentGranule.blockType == BlockType::ShortWindows3) && (std::get<SideInformationGranuleSpecialWindow>(currentGranule.window).mixedBlockFlag == false)) {//TODO: voir si mixed block pour la partie long block doit etre traitée ou non !!!
            return;
        }

        // Get current frequency line values vector
        auto &currentFrequencyLineValues = _frequencyLineValues[(granuleIndex * _header.getNumberOfChannels()) + channelIndex];

        // Browse subbands of 18 frequency lines
        unsigned int const subBandEndBound = (((currentGranule.blockType == BlockType::ShortWindows3) && (std::get<SideInformationGranuleSpecialWindow>(currentGranule.window).mixedBlockFlag == true)) ? 2 : 32);//TODO: a voir avec le todo du dessus

        for (unsigned int subBand = 1; subBand < subBandEndBound; ++subBand) {
            // Browse butterflies
            for (unsigned int butterflyIndex = 0; butterflyIndex < 8; ++butterflyIndex) {
                // Get frequency lines values to apply alias reduction
                auto const frequencyLineUp = currentFrequencyLineValues[(18 * subBand) - (1 + butterflyIndex)];
                auto const frequencyLineDown = currentFrequencyLineValues[(18 * subBand) + butterflyIndex];

                // Apply butterfly
                currentFrequencyLineValues[(18 * subBand) - (1 + butterflyIndex)] = (frequencyLineUp * Data::butterflyCoefficientCs[butterflyIndex]) - (frequencyLineDown * Data::butterflyCoefficientCa[butterflyIndex]);
                currentFrequencyLineValues[(18 * subBand) + butterflyIndex] = (frequencyLineDown * Data::butterflyCoefficientCs[butterflyIndex]) + (frequencyLineUp * Data::butterflyCoefficientCa[butterflyIndex]);
            }
        }
    }

    void Frame::applyIMDCT(SideInformationGranule const &sideInformationGranule, std::array<float, 576> const &frequencyLineValues, std::array<float, 36> &subbandValues, unsigned int const subbandIndex) const {
        unsigned int const n = ((sideInformationGranule.blockType == BlockType::ShortWindows3) && ((std::get<SideInformationGranuleSpecialWindow>(sideInformationGranule.window).mixedBlockFlag == false) || (subbandIndex >= 2))) ? 12 : 36;
        unsigned int const halfN = n / 2;   // TODO: renommer mieux les indexs et variables ici

        // Produce 36 values
        for (unsigned int i = 0; i < 36; ++i) { // TODO: toutes les constantes comme 36, 12, ... doivent etre dans des constantes
            // From 18 (6+6+6 in short block) values
            for (unsigned int k = 0; k < halfN; ++k) {
                subbandValues[i] += frequencyLineValues[(subbandIndex * 18) + ((i / n) * halfN) + k] * /*(((granule.blockType == BlockType::ShortWindows3) && ((granule.window.special.mixedBlockFlag == false) || (subbandIndex >= 2))) ? Data::imdctShortBlock[((i % 12) * halfN) + k] : Data::imdctLongBlock[(i * halfN) + k]);/*/std::cos((M_PI / (2.0f * n)) * ((2.0f * i) + 1.0f + halfN) * ((2.0f * k) + 1.0f));
            }
        }
    }

    void Frame::applyWindowing(SideInformationGranule const &sideInformationGranule, std::array<float, 36> &subbandValues, unsigned int const subbandIndex) const {
        // Get windowing values
        auto const &windowingValues = Data::windowingValuesPerBlock[static_cast<unsigned int>(sideInformationGranule.blockType)];//TODO: voir si laisser ainsi avec le cast du enum ou si avoir une methode a part et attention pour les short blocks

        // Process windowing
        for (unsigned int i = 0; i < 36; ++i) {
            subbandValues[i] *= windowingValues[i];
        }

        // Process overlapping between short blocks if necessary
        if ((sideInformationGranule.blockType == BlockType::ShortWindows3) && ((std::get<SideInformationGranuleSpecialWindow>(sideInformationGranule.window).mixedBlockFlag == false) || (subbandIndex >= 2))) {
            // Second window First Half to First window Second Half + Second window First Half
            for (unsigned int i = 12; i < 18; ++i) {
                subbandValues[i] += subbandValues[i - 6];
            }
            // Second window Second Half to Second window Second Half + Third window First Half
            for (unsigned int i = 18; i < 24; ++i) {
                subbandValues[i] += subbandValues[i + 6];
            }
            // Third window First Half to Third window Second Half
            for (unsigned int i = 24; i < 30; ++i) {
                subbandValues[i] = subbandValues[i + 6];
            }
            // First window Second Half to First window First Half (need to be here because we can't override these values before using it)
            for (unsigned int i = 6; i < 12; ++i) {
                subbandValues[i] = subbandValues[i - 6];
            }
            // First window First Half to 0 and Third window Second Half to 0 (need to be here because we can't override these values before using it)
            for (unsigned int i = 0; i < 6; ++i) {
                subbandValues[i] = 0;
                subbandValues[30 + i] = 0;
            }
        }
    }

    void Frame::applyOverlapping(std::array<float, 36> const &subbandValues, std::array<float, 576> &currentSubbandsValues, unsigned int const subbandIndex) {
        // Process overlapping
        for (unsigned int i = 0; i < ((subbandIndex < 31) ? 36 : 18); ++i) {//TODO: a voir si correct ainsi
            currentSubbandsValues[(subbandIndex * 18) + i] += subbandValues[i];
        }
    }

    void Frame::applyCompensationForFrequencyInversion(std::array<float, 576> &currentSubbandsValues, unsigned int const subbandIndex) {
        // Process frequency inversion
        // Only on odd indexes
        for (unsigned int i = 1; i < 18; i += 2) {
            // Only on odd indexes
            if ((subbandIndex % 2) == 1) {//TODO: sortir la condition de la boucle et sortir directement de la methode si pair
                currentSubbandsValues[(subbandIndex * 18) + i] *= -1.0f;
            }
        }
    }

    void Frame::applyPolyphaseFilterBank(unsigned int const granuleIndex, unsigned int const channelIndex) {
        // Get current subbands values vector
        auto &currentSubbandsValues = _subbandsValues[(granuleIndex * _header.getNumberOfChannels()) + channelIndex];

        // Get current pcm values vector
        auto &currentPcmValues = _pcmValues[(granuleIndex * _header.getNumberOfChannels()) + channelIndex];

        // Create shifted and matrixed subbands values array
        //std::array<float, 1024> shiftedAndMatrixedSubbandsValues = {};  // TODO: voir si doit etre reutilisé quand il est construit pour l'autre granule du meme canal !!!
        auto &shiftedAndMatrixedSubbandsValues = _shiftedAndMatrixedSubbandsValues[channelIndex];//TODO: voir si ainsi ou juste comme au dessus

        // Time loop
        for (unsigned int t = 0; t < 18; ++t) {
            // Apply shifting
            for (unsigned int i = 1023; i >= 64; --i) {
                shiftedAndMatrixedSubbandsValues[i] = shiftedAndMatrixedSubbandsValues[i - 64];
            }

            // Apply matrixing
            for (unsigned int i = 0; i < 64; ++i) {
                shiftedAndMatrixedSubbandsValues[i] = 0.0f;

                for (unsigned int k = 0; k < 32; ++k) {
                    shiftedAndMatrixedSubbandsValues[i] += Data::matrixingCoefficients[i][k] * currentSubbandsValues[(18 * k) + t];
                }
            }

            // Create U values
            std::array<float, 512> uValues;

            for (unsigned int i = 0; i < 8; ++i) {
                for (unsigned int j = 0; j < 32; ++j) {
                    uValues[(64 * i) + j] = shiftedAndMatrixedSubbandsValues[(128 * i) + j];
                    uValues[(64 * i) + 32 + j] = shiftedAndMatrixedSubbandsValues[(128 * i) + 96 + j];
                }
            }

            // Create window values
            std::array<float, 512> windowValues;

            for (unsigned int i = 0; i < 512; ++i) {
                windowValues[i] = uValues[i] * Data::synthesisCoefficients[i];
            }

            // Calculate samples
            for (unsigned int j = 0; j < 32; ++j) {
                currentPcmValues[(32 * t) + j] = 0.0f;

                for (unsigned int i = 0; i < 16; ++i) {
                    currentPcmValues[(32 * t) + j] += windowValues[(32 * i) + j];
                }
            }
        }
    }

}
