#ifndef MP3_DECODER_S_HPP
#define MP3_DECODER_S_HPP


template <class TFunction>
void Decoder::browseFramesHeader(std::istream &inputStream, TFunction &&browseFunc) const {
    // Reset stream
    inputStream.clear();
    inputStream.seekg(0);

    // Create array for header
    std::array<uint8_t, Frame::Header::headerSize> headerData;

    // While inputStream is good
    while (inputStream) {
        // Exit if we can't synchronize to next frame
        if (synchronizeToNextFrame(inputStream, headerData, 0xFE, 0xFA) == false) {
            // Can't find another frame
            break;
        }
        
        // Create header
        Frame::Header header(headerData, 0xFE, 0xFA);

        // Call browseFund and exit if returns false
        if (browseFunc(header) == false) {
            break;
        }

        // Pass frame size bytes in stream (minus header size)
        inputStream.seekg(header.getFrameLength() - headerData.size(), std::ios_base::cur);
    }
}

#endif /* MP3_DECODER_S_HPP */
