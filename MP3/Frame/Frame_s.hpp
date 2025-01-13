#ifndef MP3_FRAME_FRAME_S_HPP
#define MP3_FRAME_FRAME_S_HPP


template <class TFunction>
Frame::Frame(Header const &header, SideInformation const &sideInformation, unsigned int const ancillaryDataSizeInBits, std::vector<uint8_t> const &data, std::array<std::array<float, 576>, 2> &blocksSubbandsOverlappingValues, std::array<std::array<float, 1024>, 2> &shiftedAndMatrixedSubbandsValues, TFunction &&errorFunction) : _header(header), _sideInformation(sideInformation), _data(data), _dataBitIndex(0), _blocksSubbandsOverlappingValues(blocksSubbandsOverlappingValues), _shiftedAndMatrixedSubbandsValues(shiftedAndMatrixedSubbandsValues) {
    // Resize vectors
    for (unsigned int index = 0; index < 2; ++index) {
        // Resize scaleFactors vector (variable number of channels)
        _scaleFactors[index].resize(_header.getNumberOfChannels());

        // Resize frequencyLineValues vector (variable number of channels)
        _frequencyLineValues[index].resize(_header.getNumberOfChannels());

        // Resize startingRzeroFrequencyLineIndexes vector (variable number of channels)
        _startingRzeroFrequencyLineIndexes[index].resize(2 * _header.getNumberOfChannels());
    }

    // Resize pcmValues vector (Keep 2 granules by array but variable number of channels)
    _pcmValues.resize(_header.getNumberOfChannels());

    // Extract main data
    extractMainData(std::forward<TFunction>(errorFunction));
}

template <class TFunction>
void Frame::extractMainData(TFunction &&errorFunction) {
    // Browse granules
    for (unsigned int granuleIndex = 0; granuleIndex < 2; ++granuleIndex) {
        // Browse channels
        for (unsigned int channelIndex = 0; channelIndex < _header.getNumberOfChannels(); ++channelIndex) {
            // Save dataBitIndex before start extracting granule data
            auto const granuleStartDataBitIndex = _dataBitIndex;

            // Extract scale factors
            extractScaleFactors(granuleIndex, channelIndex);

            // Extract frequency line values
            extractFrequencyLineValues(granuleIndex, channelIndex, granuleStartDataBitIndex, std::forward<TFunction>(errorFunction));

            // Requantize frequency line values
            requantizeFrequencyLineValues(granuleIndex, channelIndex);

            // Reorder short windows
            reorderShortWindows(granuleIndex, channelIndex);
        }

        // Process stereo (need to be done when all channels are decoded)
        processStereo(granuleIndex);
        
        // What is following can't be done before finishing processStereo, it's why we separated it to another loop

        // Browse channels
        for (unsigned int channelIndex = 0; channelIndex < _header.getNumberOfChannels(); ++channelIndex) {
            // Synthesis FilterBank
            synthesisFilterBank(granuleIndex, channelIndex);
        }
    }
}

template <class TFunction>
void Frame::extractFrequencyLineValues(unsigned int const granuleIndex, unsigned int const channelIndex, unsigned int const granuleStartDataBitIndex, TFunction &&errorFunction) {
    // Get current granule
    auto const &currentGranule = _sideInformation.getGranule(granuleIndex, channelIndex);

    // Get current frequency line values vector
    auto &currentFrequencyLineValues = _frequencyLineValues[granuleIndex][channelIndex];

    // Get current starting Rzero frequency line index
    auto &currentStartingRzeroFrequencyLineIndex = _startingRzeroFrequencyLineIndexes[granuleIndex][channelIndex];

    // Browse all frequency lines
    unsigned int frequencyLineIndex = 0;

    // Start with bigValues
    for (; (frequencyLineIndex < (currentGranule.bigValues * 2)) && (frequencyLineIndex < 576); frequencyLineIndex += 2) {//TODO: 576 dans une const
        // Get current region
        auto const currentRegion = getBigValuesRegionForFrequencyLineIndex(currentGranule, frequencyLineIndex);

        // Get huffman table index
        auto const huffmanTableIndex = currentGranule.tableSelect[currentRegion];

        Data::QuantizedValuePair decodedValue = { 0, 0 };

        // If huffman table is zero, don't read in stream but set all the region to 0
        if (huffmanTableIndex > 0) {
            // Get huffman table
            auto const &huffmanData = Data::bigValuesData[huffmanTableIndex];

            // Decode data
            auto decodedValuePair = decodeHuffmanCode(currentGranule.par23Length, granuleStartDataBitIndex, huffmanData.table, huffmanData.maxEntryLength);

            // Verify if no error
            if (decodedValuePair.first.has_value() == false) {
                if (errorFunction(Error::HuffmanCodeNotFound(*this, decodedValuePair.second, decodedValue, frequencyLineIndex)) == false) {
                    throw Error::HuffmanCodeNotFound(*this, decodedValuePair.second, decodedValue, frequencyLineIndex);
                }
            } else {
                // Get value
                decodedValue = *(decodedValuePair.first);
            }

            // Correct data
            decodedValue.x = huffmanCodeApplyLinbitsAndSign(decodedValue.x, huffmanData.linbits);
            decodedValue.y = huffmanCodeApplyLinbitsAndSign(decodedValue.y, huffmanData.linbits);
        }

        // Add to currentFrequencyLineValues
        currentFrequencyLineValues[frequencyLineIndex] = decodedValue.x;
        currentFrequencyLineValues[frequencyLineIndex + 1] = decodedValue.y;
    }
        
    // Then we have Count1 values
    for (; ((_dataBitIndex - granuleStartDataBitIndex) < currentGranule.par23Length) && (frequencyLineIndex < (576 - 3)); frequencyLineIndex += 4) {
        Data::QuantizedValueQuadruple decodedValue = { 0, 0, 0, 0 };

        // Decode value if table is A
        if (currentGranule.isCount1TableB == false) {
            // Decode data
            auto decodedValuePair = decodeHuffmanCode(currentGranule.par23Length, granuleStartDataBitIndex, Data::count1TableA, Data::count1TableAMaxEntryLength);

            // Verify if no error
            if (decodedValuePair.first.has_value() == false) {
                if (errorFunction(Error::HuffmanCodeNotFound(*this, decodedValuePair.second, decodedValue, frequencyLineIndex)) == false) {
                    throw Error::HuffmanCodeNotFound(*this, decodedValuePair.second, decodedValue, frequencyLineIndex);
                }
            } else {
                // Get value
                decodedValue = *(decodedValuePair.first);
            }
        }
        // Simply get 4 bits and invert them if table is B
        else {
            // Get data
            decodedValue.v = Helper::getBitsAtIndex<unsigned int>(_data, _dataBitIndex, 1) ^ 0x1;
            decodedValue.w = Helper::getBitsAtIndex<unsigned int>(_data, _dataBitIndex, 1) ^ 0x1;
            decodedValue.x = Helper::getBitsAtIndex<unsigned int>(_data, _dataBitIndex, 1) ^ 0x1;
            decodedValue.y = Helper::getBitsAtIndex<unsigned int>(_data, _dataBitIndex, 1) ^ 0x1;
        }

        // Correct data
        decodedValue.v = huffmanCodeApplySign(decodedValue.v);
        decodedValue.w = huffmanCodeApplySign(decodedValue.w);
        decodedValue.x = huffmanCodeApplySign(decodedValue.x);
        decodedValue.y = huffmanCodeApplySign(decodedValue.y);

        // Add to currentFrequencyLineValues
        currentFrequencyLineValues[frequencyLineIndex] = decodedValue.v;
        currentFrequencyLineValues[frequencyLineIndex + 1] = decodedValue.w;
        currentFrequencyLineValues[frequencyLineIndex + 2] = decodedValue.x;
        currentFrequencyLineValues[frequencyLineIndex + 3] = decodedValue.y;
    }

    // Then we have Rzero values
    // Save start of Rzero values
    currentStartingRzeroFrequencyLineIndex = frequencyLineIndex;

    for (; frequencyLineIndex < 576; ++frequencyLineIndex) {
        // Add 0 to currentFrequencyLineValues
        currentFrequencyLineValues[frequencyLineIndex] = 0;
    }
}

template <class TValueType>
std::pair<std::optional<TValueType>, unsigned int> Frame::decodeHuffmanCode(unsigned int const granulePart23LengthInBits, unsigned int const granuleStartDataBitIndex, std::unordered_map<unsigned int, TValueType> const &huffmanTable, unsigned int const maxEntryBitLength) {
    // Browse huffman code
    unsigned int code = 1;

    for (unsigned int size = 0; (size < maxEntryBitLength) && ((_dataBitIndex - granuleStartDataBitIndex) < granulePart23LengthInBits); ++size) {
        code <<= 1;
        code |= Helper::getBitsAtIndex<unsigned int>(_data, _dataBitIndex, 1);

        auto const &pair = huffmanTable.find(code);
        if (pair != std::end(huffmanTable)) {
            return std::make_pair(pair->second, code);
        }
    }

    return std::make_pair(std::nullopt, code);
}

template <class TScaleFactorBandTable>
unsigned int Frame::getScaleFactorBandIndexForFrequencyLineIndex(TScaleFactorBandTable const &scaleFactorBands, unsigned int const frequencyLineIndex) const {
    // TODO: assert sur frequencyLineIndex < 576 : ATTENTION POUR LES SHORT BLOCKS c'est < 192
    // Browse all scale factor bands
    for (unsigned int scaleFactorBandIndex = 0; scaleFactorBandIndex < scaleFactorBands.size(); ++scaleFactorBandIndex) {
        // If frequency line index is in current scale factor band
        if (frequencyLineIndex <= scaleFactorBands[scaleFactorBandIndex]) {
            // Return scaleFactorBandIndex
            return scaleFactorBandIndex;
        }
    }

    // Not found
    return -1;//TODO: exception normalement
}

template <typename TValue>
Error::HuffmanCodeNotFound<TValue>::HuffmanCodeNotFound(Frame &frame, unsigned int huffmanCodedValue, TValue &huffmanDecodedValue, unsigned int frequencyLineIndex) : FrameException(frame), _huffmanCodedValue(huffmanCodedValue), _huffmanDecodedValue(huffmanDecodedValue), _frequencyLineIndex(frequencyLineIndex) {
}

template <typename TValue>
unsigned int Error::HuffmanCodeNotFound<TValue>::getHuffmanCodedValue() const {
    return _huffmanCodedValue;
}

template <typename TValue>
TValue &Error::HuffmanCodeNotFound<TValue>::getHuffmanDecodedValue() const {
    return _huffmanDecodedValue;
}

template <typename TValue>
unsigned int Error::HuffmanCodeNotFound<TValue>::getFrequencyLineIndex() const {
    return _frequencyLineIndex;
}

#endif /* MP3_FRAME_FRAME_S_HPP */
