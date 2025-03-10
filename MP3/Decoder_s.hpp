#ifndef MP3_DECODER_S_HPP
#define MP3_DECODER_S_HPP


template <class TFunction>
void Decoder::browseFramesHeader(TFunction &&browseFunc) {
    unsigned int frameIndex = 0;

    // Frames loop
    for (;;) {
        // Get next frame header
        auto const frameHeader = tryToGetFrameHeaderAtIndex(frameIndex);

        // Exit loop if no frame header left
        if (frameHeader.has_value() == false) {
            break;
        }

        // Call browseFunc and exit if returns false
        if (browseFunc(*frameHeader) == false) {
            break;
        }

        // Increment frameIndex
        ++frameIndex;
    }
}

template <class TFunction>
Frame::Frame Decoder::getFrameAtIndex(unsigned int const frameIndex, TFunction &&errorFunction) {
    // Get frame header at frameIndex
    auto frameHeader = tryToGetFrameHeaderAtIndex(frameIndex);

    // If no frame header, error
    if (frameHeader.has_value() == false) {
        throw Error::FrameNotFound(*this, frameIndex);//TODO: a voir
    }

    // Get stored CRC if needed
    auto const crcStored = getCRCIfExist((*frameHeader));

    // Get frame side information data
    auto frameSideInformationData = Helper::getDataFromStream((*_inputStream), (*frameHeader).getSideInformationSize());

    // Check CRC if needed
    if ((*frameHeader).isCRCProtected() == true) {
        auto const crcCalculated = calculateCRC((*frameHeader).getData(), frameSideInformationData);

        // Check CRC
        if (crcStored != crcCalculated) {
            // Call error function
            if (errorFunction(Error::FrameCRCIncorrect(*this, frameIndex, crcStored, crcCalculated, (*frameHeader), (*frameHeader).getData(), frameSideInformationData)) == false) {
                throw Error::FrameCRCIncorrect(*this, frameIndex, crcStored, crcCalculated, std::as_const(*frameHeader), (*frameHeader).getData(), std::as_const(frameSideInformationData));//TODO: a voir
            }
        }
    }

    // Create frame side information
    Frame::SideInformation const frameSideInformation((*frameHeader), frameSideInformationData, std::forward<TFunction>(errorFunction));

    // Get frame data from bit reservoir
    std::vector<uint8_t> frameData((frameSideInformation.getMainDataSizeInBits() / 8) + (((frameSideInformation.getMainDataSizeInBits() % 8) != 0) ? 1 : 0), 0);
    auto const frameDataResult = getFrameDataFromBitReservoir(frameIndex, frameSideInformation, frameData);

    // If no frame data, error
    if (frameDataResult == false) {
        // Call error function
        if (errorFunction(Error::FrameNoData(*this, frameIndex, frameData)) == false) {//TODO: passer aussi le frameHeaderData, le frameSideInformationData, le crc ?
            throw Error::FrameNoData(*this, frameIndex, std::as_const(frameData));//TODO: a voir
        }
    }

    // Get frame ancillary data size in bits
    unsigned int const frameAncillaryDataSizeInBits = 0; //TODO: changer, attention dans le cas ou toutes les données d'une frame sont dans la frame précédente

    // Create frame
    return Frame::Frame((*frameHeader), frameSideInformation, frameAncillaryDataSizeInBits, frameData, _framesBlocksSubbandsOverlappingValues, _framesShiftedAndMatrixedSubbandsValues, std::forward<TFunction>(errorFunction));
}

#endif /* MP3_DECODER_S_HPP */
