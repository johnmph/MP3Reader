
#include <cmath>
#include "Frame.hpp"
#include "Data/Header.hpp"
#include "Data/ScaleFactorBands.hpp"
#include "Data/SynthesisFilterBank.hpp"
#include "Data/SynthesisWindow.hpp"

//TODO: bien verifier pour les conditions et le code executé pour les types de block, si mixed ou pas, ...
namespace MP3::Frame {

    unsigned int Frame::getNumberOfChannels() const {
        return _header.getNumberOfChannels();
    }

    unsigned int Frame::getBitrate() const {
        return _header.getBitrate();
    }

    unsigned int Frame::getSamplingRate() const {
        return Data::samplingRates[_header.getSamplingRateIndex()];
    }

    ChannelMode Frame::getChannelMode() const {
        return _header.getChannelMode();
    }

    bool Frame::isCRCProtected() const {
        return _header.isCRCProtected();
    }

    bool Frame::isIntensityStereo() const {
        return _header.isIntensityStereo();
    }

    bool Frame::isMSStereo() const {
        return _header.isMSStereo();
    }

    bool Frame::isCopyrighted() const {
        return _header.isCopyrighted();
    }

    bool Frame::isOriginal() const {
        return _header.isOriginal();
    }

    std::vector<uint8_t> const &Frame::getAncillaryBits() const {
        //TODO: terminer
        return {};
    }

    std::array<float, 1152> const &Frame::getPCMSamples(unsigned int const channelIndex) const {
        return _pcmValues[channelIndex];
    }

    void Frame::extractScaleFactors(unsigned int const granuleIndex, unsigned int const channelIndex) {
        //TODO: If short windows are switched on, i.e. block-type==2 for one of the granules, then scfsi is always 0 for this frame : voir si c'est certifié par l'encoder ou si c'est ici qu'il faut le mettre a 0
        // Get current granule
        auto const &currentGranule = _sideInformation.getGranule(granuleIndex, channelIndex);

        // Get current scaleFactors vector
        auto &currentScaleFactors = _scaleFactors[granuleIndex][channelIndex];

        // If we have 3 short windows
        if (currentGranule.blockType == BlockType::ShortWindows3) {
            // If we have mixed block
            if (std::get<SideInformationGranuleSpecialWindow>(currentGranule.window).mixedBlockFlag == true) {
                // Browse scaleFactor bands 0 to 7 for long window
                for (unsigned int scaleFactorBandIndex = 0; scaleFactorBandIndex < 8; ++scaleFactorBandIndex) {
                    currentScaleFactors.longWindow[scaleFactorBandIndex] = Helper::getBitsAtIndex<unsigned int>(_data, _dataBitIndex, currentGranule.sLen1);
                }

                // Browse scaleFactor bands 3 to 11 for short window
                for (unsigned int scaleFactorBandIndex = 3; scaleFactorBandIndex < 12; ++scaleFactorBandIndex) {
                    // Browse windows
                    for (unsigned int windowIndex = 0; windowIndex < 3; ++windowIndex) {
                        currentScaleFactors.shortWindow[scaleFactorBandIndex][windowIndex] = Helper::getBitsAtIndex<unsigned int>(_data, _dataBitIndex, (scaleFactorBandIndex < 6) ? currentGranule.sLen1 : currentGranule.sLen2);
                    }
                }
            } else {
                // Browse scaleFactor bands 0 to 11 for short window
                for (unsigned int scaleFactorBandIndex = 0; scaleFactorBandIndex < 12; ++scaleFactorBandIndex) {
                    // Browse windows
                    for (unsigned int windowIndex = 0; windowIndex < 3; ++windowIndex) {
                        currentScaleFactors.shortWindow[scaleFactorBandIndex][windowIndex] = Helper::getBitsAtIndex<unsigned int>(_data, _dataBitIndex, (scaleFactorBandIndex < 6) ? currentGranule.sLen1 : currentGranule.sLen2);
                    }
                }
            }
        } else {
            // Browse scaleFactor bands 0 to 20 for long window
            for (unsigned int scaleFactorBandIndex = 0; scaleFactorBandIndex < 21; ++scaleFactorBandIndex) {
                // If we don't share or if we are in first granule
                if ((_sideInformation.getScaleFactorShare(channelIndex, getScaleFactorShareGroupForScaleFactorBand(scaleFactorBandIndex)) == false) || (granuleIndex == 0)) {
                    currentScaleFactors.longWindow[scaleFactorBandIndex] = Helper::getBitsAtIndex<unsigned int>(_data, _dataBitIndex, (scaleFactorBandIndex < 11) ? currentGranule.sLen1 : currentGranule.sLen2);
                } else {
                    // Share with first granule
                    currentScaleFactors.longWindow[scaleFactorBandIndex] = _scaleFactors[0][channelIndex].longWindow[scaleFactorBandIndex];
                }
            }
        }
    }
    
    void Frame::requantizeFrequencyLineValues(unsigned int const granuleIndex, unsigned int const channelIndex) {
        // Get current granule
        auto const &currentGranule = _sideInformation.getGranule(granuleIndex, channelIndex);

        // Get current frequency line values vector
        auto &currentFrequencyLineValues = _frequencyLineValues[granuleIndex][channelIndex];

        // Browse frequency lines
        for (unsigned int frequencyLineIndex = 0; frequencyLineIndex < 576; ++frequencyLineIndex) {//TODO: toutes les references a 576 doivent etre retirées et remplacées par un const
            // Get gainCorrection
            unsigned int const gainCorrection = getSubblockGainForFrequencyLineIndex(currentGranule, frequencyLineIndex);

            // Get scaleFactor
            unsigned int const scaleFactor = getScaleFactorForFrequencyLineIndex(granuleIndex, channelIndex, currentGranule, frequencyLineIndex);

            // Get current value sign
            float const currentValueSign = (currentFrequencyLineValues[frequencyLineIndex] >= 0.0f) ? 1.0f : -1.0f;

            // Calculate power gain
            float const powerGain = std::pow(2.0f, static_cast<int>(currentGranule.globalGain - (210 + gainCorrection)) / 4.0f);//TODO: voir si 210 sera dans un const

            // Calculate power scale factor
            float const powerScaleFactor = std::pow(2.0f, -(currentGranule.scaleFacScale * scaleFactor));

            // Requantize current value
            currentFrequencyLineValues[frequencyLineIndex] = currentValueSign * std::pow(std::abs(currentFrequencyLineValues[frequencyLineIndex]), 4.0f / 3.0f) * powerGain * powerScaleFactor;
        }
    }

    void Frame::reorderShortWindows(unsigned int const granuleIndex, unsigned int const channelIndex) {//TODO: voir si le reorder est correct ainsi : OK normalement, juste pas testé si mixedBlockFlag !
    // TODO: a la place de calculer les indexs de reorder ici, avoir des tableaux constants calculés au démarrage avec une methode constexpr ! et ici on utilisera les indexs du bon tableau reorder pour reorder les données
        // Get current granule
        auto const &currentGranule = _sideInformation.getGranule(granuleIndex, channelIndex);

        // Don't reorder other than short block
        if (currentGranule.blockType != BlockType::ShortWindows3) {
            return;
        }

        // Get current frequency line values vector
        auto &currentFrequencyLineValues = _frequencyLineValues[granuleIndex][channelIndex];
        auto savedCurrentFrequencyLineValues = currentFrequencyLineValues;
        
        // Browse frequency lines
        unsigned int startFrequencyLineForCurrentScaleFactorBand = 0;
        unsigned int reorderedIndex = 0;

        for (unsigned int frequencyLineIndex = ((std::get<SideInformationGranuleSpecialWindow>(currentGranule.window).mixedBlockFlag == true) ? 36 : 0); frequencyLineIndex < 576; ++frequencyLineIndex) {//TODO: toutes les references a 576 doivent etre retirées et remplacées par un const et 36 aussi
            // Get current scaleFactor band index
            auto const currentScaleFactorBandIndex = getScaleFactorBandIndexForFrequencyLineIndex(Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()], frequencyLineIndex / 3);

            // Get current scaleFactor band frequency line count
            int const currentScaleFactorBandFrequencyLineCount = Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()][currentScaleFactorBandIndex] - ((currentScaleFactorBandIndex > 0) ? Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()][currentScaleFactorBandIndex - 1] : -1);

            // Reorder
            currentFrequencyLineValues[frequencyLineIndex] = savedCurrentFrequencyLineValues[startFrequencyLineForCurrentScaleFactorBand + reorderedIndex];

            // Update reorderedIndex
            reorderedIndex += currentScaleFactorBandFrequencyLineCount;

            if (reorderedIndex >= (currentScaleFactorBandFrequencyLineCount * 3)) {
                reorderedIndex -= (currentScaleFactorBandFrequencyLineCount * 3) - 1;

                // The current scale factor band index is over, go to next
                if (reorderedIndex == currentScaleFactorBandFrequencyLineCount) {
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
        auto const &currentFrequencyLineValues = _frequencyLineValues[granuleIndex][channelIndex];

        // Get current subbands values vector
        std::array<float, 576> subbandsValues = {};

        // Browse subbands
        for (unsigned int subbandIndex = 0; subbandIndex < 32; ++subbandIndex) {
            // Create temporary array to keep current subband values
            std::array<float, 36> currentSubbandValues = {};

            // Apply IMDCT
            applyIMDCT(currentGranule, currentFrequencyLineValues, currentSubbandValues, subbandIndex);

            // Apply windowing
            applyWindowing(currentGranule, currentSubbandValues, subbandIndex);

            // Apply overlapping
            applyOverlapping(channelIndex, currentSubbandValues, subbandsValues, subbandIndex);

            // Apply compensation for frequency inversion
            applyCompensationForFrequencyInversion(subbandsValues, subbandIndex);
        }

        // Apply polyphase filterbank
        applyPolyphaseFilterBank(granuleIndex, channelIndex, subbandsValues);
    }

    unsigned int Frame::getScaleFactorShareGroupForScaleFactorBand(unsigned int const scaleFactorBand) const {
        // Browse all scale factor share groups
        for (unsigned int scaleFactorShareGroupIndex = 1; scaleFactorShareGroupIndex < 5; ++scaleFactorShareGroupIndex) {
            // If scaleFactorBand is in current group
            if ((scaleFactorBand >= Data::scaleFactorBandsPerScaleFactorShareGroup[scaleFactorShareGroupIndex - 1]) && (scaleFactorBand < Data::scaleFactorBandsPerScaleFactorShareGroup[scaleFactorShareGroupIndex])) {
                // Return group index
                return scaleFactorShareGroupIndex - 1;
            }
        }

        // Not found
        return -1;  // TODO: attention on retourne -1 en unsigned int : thrower une exception normalement ou faire un assert au début de la methode sur scaleFactorBand < 21
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

    int Frame::huffmanCodeApplyLinbitsAndSign(int value, unsigned int const linbits) {
        //TODO: thrower si on depasse la taille de la granule
        // Add linbits if necessary
        if ((value == 15) && (linbits > 0)) {
            value += Helper::getBitsAtIndex<unsigned int>(_data, _dataBitIndex, linbits);
        }

        // Correct sign
        value = huffmanCodeApplySign(value);

        return value;
    }

    int Frame::huffmanCodeApplySign(int value) {
        //TODO: thrower si on depasse la taille de la granule
        // Correct sign if necessary
        if (value != 0) {
            value *= (Helper::getBitsAtIndex<unsigned int>(_data, _dataBitIndex, 1) == 0) ? 1 : -1;
        }

        return value;
    }

    unsigned int Frame::getSubblockGainForFrequencyLineIndex(SideInformationGranule const &currentGranule, unsigned int const frequencyLineIndex) const {
        // No subblock gain for long block
        if (isFrequencyLineIndexInShortBlock(currentGranule, frequencyLineIndex) == false) {
            return 0;
        }

        // Get scaleFactor band index
        auto const scaleFactorBandIndex = getScaleFactorBandIndexForFrequencyLineIndex(Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()], frequencyLineIndex / 3);
        
        // Return 0 if scaleFactor band index not found else return subblockGain * 8
        return (scaleFactorBandIndex != 12) ? (8 * std::get<SideInformationGranuleSpecialWindow>(currentGranule.window).subblockGain[getCurrentWindowIndexForFrequencyLineIndex(scaleFactorBandIndex, frequencyLineIndex)]) : 0;
    }

    unsigned int Frame::getCurrentWindowIndexForFrequencyLineIndex(unsigned int const scaleFactorBandIndex, unsigned int const frequencyLineIndex) const {
        // TODO: assert sur scaleFactorBandIndex < 12

        // Get currentWindowIndex
        int const currentScaleFactorBandMaxFrequencyLine = Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()][scaleFactorBandIndex];
        int const currentScaleFactorBandLastMaxFrequencyLine = (scaleFactorBandIndex > 0) ? Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()][scaleFactorBandIndex - 1] : -1;

        return (frequencyLineIndex - ((currentScaleFactorBandLastMaxFrequencyLine + 1) * 3)) / (currentScaleFactorBandMaxFrequencyLine - currentScaleFactorBandLastMaxFrequencyLine);
    }

    unsigned int Frame::getScaleFactorForFrequencyLineIndex(unsigned int const granuleIndex, unsigned int const channelIndex, SideInformationGranule const &sideInformationGranule, unsigned int const frequencyLineIndex) const {
        // Get current scaleFactors vector
        auto &currentScaleFactors = _scaleFactors[granuleIndex][channelIndex];

        // Short block
        if (isFrequencyLineIndexInShortBlock(sideInformationGranule, frequencyLineIndex)) {
            // Get scaleFactor band index
            auto const scaleFactorBandIndex = getScaleFactorBandIndexForFrequencyLineIndex(Data::frequencyLinesPerScaleFactorBandShortBlock[_header.getSamplingRateIndex()], frequencyLineIndex / 3);

            // Get scale factor
            return (scaleFactorBandIndex != 12) ? currentScaleFactors.shortWindow[scaleFactorBandIndex][getCurrentWindowIndexForFrequencyLineIndex(scaleFactorBandIndex, frequencyLineIndex)] : 0;//TODO: a voir pour -1
        }

        // Long block
        // Get scale factor
        auto const scaleFactorBandIndex = getScaleFactorBandIndexForFrequencyLineIndex(Data::frequencyLinesPerScaleFactorBandLongBlock[_header.getSamplingRateIndex()], frequencyLineIndex);
        return (scaleFactorBandIndex != 21) ? (currentScaleFactors.longWindow[scaleFactorBandIndex] + (sideInformationGranule.preflag * Data::pretabByScaleFactorBand[scaleFactorBandIndex])) : 0; // TODO: a voir pour -1
    }

    bool Frame::isFrequencyLineIndexInShortBlock(SideInformationGranule const &sideInformationGranule, unsigned int const frequencyLineIndex) const {
        return (sideInformationGranule.blockType == BlockType::ShortWindows3) && ((std::get<SideInformationGranuleSpecialWindow>(sideInformationGranule.window).mixedBlockFlag == false) || (frequencyLineIndex >= 36));
    }

    void Frame::processMSStereo(unsigned int const granuleIndex) {
        // TODO: tester si les blocks des 2 canaux sont de memes types sinon throw exception ?

        // Get intensityStereoBound
        auto const intensityStereoBound = (_header.isIntensityStereo() == true) ? getIntensityStereoBound(granuleIndex) : 576;

        // Get current frequency line values vector
        auto &currentFrequencyLineValuesLeft = _frequencyLineValues[granuleIndex][0];
        auto &currentFrequencyLineValuesRight = _frequencyLineValues[granuleIndex][1];

        // Process
        for (unsigned int frequencyLineIndex = 0; frequencyLineIndex < intensityStereoBound; ++frequencyLineIndex) {
            auto const midValue = currentFrequencyLineValuesLeft[frequencyLineIndex];
            auto const sideValue = currentFrequencyLineValuesRight[frequencyLineIndex];

            currentFrequencyLineValuesLeft[frequencyLineIndex] = (midValue + sideValue) / std::sqrt(2.0f);
            currentFrequencyLineValuesRight[frequencyLineIndex] = (midValue - sideValue) / std::sqrt(2.0f);
        }
    }

    void Frame::processIntensityStereo(unsigned int const granuleIndex) {//TODO: a tester avec un autre fichier car il n'utilise pas l'intensity stereo
        // Get current granule right channel
        auto const &currentGranuleRight = _sideInformation.getGranule(granuleIndex, 1);

        // Get current frequency line values vector
        auto &currentFrequencyLineValuesLeft = _frequencyLineValues[granuleIndex][0];
        auto &currentFrequencyLineValuesRight = _frequencyLineValues[granuleIndex][1];

        // Process
        for (unsigned int frequencyLineIndex = getIntensityStereoBound(granuleIndex); frequencyLineIndex < 576; ++frequencyLineIndex) {
            // Get isPosSB which is scaleFactor of the current scaleFactorBand group
            auto const isPosSB = getScaleFactorForFrequencyLineIndex(granuleIndex, 1, currentGranuleRight, frequencyLineIndex);    // TODO: voir si c'est bon

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

    unsigned int Frame::getIntensityStereoBound(unsigned int const granuleIndex) const {//TODO: attention avec currentScaleFactorBandIndex quand == -1
        // Get current granule right channel
        auto const &currentGranuleRight = _sideInformation.getGranule(granuleIndex, 1);

        // Get current starting Rzero frequency line index (right channel)
        auto const currentStartingRzeroFrequencyLineIndex = _startingRzeroFrequencyLineIndexes[granuleIndex][1] - 1;

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
        auto &currentFrequencyLineValues = _frequencyLineValues[granuleIndex][channelIndex];

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
        unsigned int const halfN = n / 2;

        // Produce 36 values
        for (unsigned int i = 0; i < 36; ++i) { // TODO: toutes les constantes comme 36, 12, ... doivent etre dans des constantes
            // From 18 (6+6+6 in short block) values
            for (unsigned int k = 0; k < halfN; ++k) {
                subbandValues[i] += frequencyLineValues[(subbandIndex * 18) + ((i / n) * halfN) + k] * /*(((sideInformationGranule.blockType == BlockType::ShortWindows3) && ((std::get<SideInformationGranuleSpecialWindow>(sideInformationGranule.window).mixedBlockFlag == false) || (subbandIndex >= 2))) ? Data::imdctShortBlock[((i % 12) * halfN) + k] : Data::imdctLongBlock[(i * halfN) + k]);/*/std::cos((M_PI / (2.0f * n)) * ((2.0f * i) + 1.0f + halfN) * ((2.0f * k) + 1.0f));
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

    void Frame::applyOverlapping(unsigned int const channelIndex, std::array<float, 36> const &currentSubbandValues, std::array<float, 576> &subbandsValues, unsigned int const subbandIndex) const {
        // Get blocks subbands overlapping values array for channel index
        auto &blocksSubbandsOverlappingValues = _blocksSubbandsOverlappingValues[channelIndex];

        // Process overlapping
        for (unsigned int i = 0; i < 18; ++i) {
            // Overlap current first values with previous block last values
            subbandsValues[(subbandIndex * 18) + i] = currentSubbandValues[i] + blocksSubbandsOverlappingValues[(subbandIndex * 18) + i];

            // Save current last values
            blocksSubbandsOverlappingValues[(subbandIndex * 18) + i] = currentSubbandValues[18 + i];
        }
    }

    void Frame::applyCompensationForFrequencyInversion(std::array<float, 576> &subbandsValues, unsigned int const subbandIndex) const {
        // Only process odd indexes
        if ((subbandIndex % 2) == 0) {
            return;
        }

        // Process frequency inversion
        // Only on odd indexes
        for (unsigned int i = 1; i < 18; i += 2) {
            subbandsValues[(subbandIndex * 18) + i] *= -1.0f;
        }
    }

    void Frame::applyPolyphaseFilterBank(unsigned int const granuleIndex, unsigned int const channelIndex, std::array<float, 576> &subbandsValues) {
        // Get current pcm values vector
        auto &currentPcmValues = _pcmValues[channelIndex];

        // Get shifted and matrixed subbands values array for channel index
        auto &shiftedAndMatrixedSubbandsValues = _shiftedAndMatrixedSubbandsValues[channelIndex];

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
                    shiftedAndMatrixedSubbandsValues[i] += Data::matrixingCoefficients[i][k] * subbandsValues[(18 * k) + t];
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
                currentPcmValues[(granuleIndex * 576) + (32 * t) + j] = 0.0f;

                for (unsigned int i = 0; i < 16; ++i) {
                    currentPcmValues[(granuleIndex * 576) + (32 * t) + j] += windowValues[(32 * i) + j];
                }
            }
        }
    }

}
