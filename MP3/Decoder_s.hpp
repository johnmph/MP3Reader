#ifndef MP3_DECODER_S_HPP
#define MP3_DECODER_S_HPP

/*
template <class TFunction>
void Decoder::browseFramesHeader(std::istream &inputStream, TFunction &&browseFunc) const {
    // Reset stream
    inputStream.clear();
    inputStream.seekg(0);

    // Create array for header
    std::array<uint8_t, Frame::Header::headerSize> headerData;

    // While inputStream is good
    while (inputStream) {
        // Exit if we can't read next frame header data
        if (tryToReadNextFrameHeaderData(inputStream, headerData) == false) {
            // Can't find another frame
            break;
        }
        
        // Create header
        Frame::Header header(headerData, _versionMask, _versionValue);

        // Call browseFund and exit if returns false
        if (browseFunc(header) == false) {
            break;
        }

        // Pass frame size bytes in stream (minus header size)
        inputStream.seekg(header.getFrameLength() - headerData.size(), std::ios_base::cur);
    }
}*/

template <class TFunction>
void Decoder::browseFramesHeader(std::istream &inputStream, TFunction &&browseFunc) {
    unsigned int frameIndex = 0;

    // Frames loop
    for (;;) {
        // Get next frame header
        auto const frameHeader = tryToGetFrameHeaderAtIndex(inputStream, frameIndex);

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


#endif /* MP3_DECODER_S_HPP */
