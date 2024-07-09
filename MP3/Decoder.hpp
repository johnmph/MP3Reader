#ifndef MP3_DECODER_HPP
#define MP3_DECODER_HPP

#include <cstdint>
#include <istream>
#include <vector>
#include "Frame/Header.hpp"
#include "Frame/Frame.hpp"


namespace MP3 {
    
    //template <class TStream>//TODO: si on veut le stream géré ici
    struct Decoder {

        // TODO: avoir un constructor avec plusieurs parametres (ou une struct Configuration) et avoir par exemple le nombre de frames a lire pour etre certain que c'est un bon fichier MP3
        Decoder();

        template <class TFunction>
        void browseFramesHeader(std::istream &inputStream, TFunction &&browseFunc) const;

        bool isValidFormat(std::istream &inputStream, unsigned int const numberOfFramesForValidFormat) const;
        unsigned int getNumberOfFrames(std::istream &inputStream) const;
        Frame::Frame getFrameAtIndex(std::istream &inputStream, unsigned int const frameIndex);
        
    private:

        struct FrameEntry {
            unsigned int positionInBytes;
            unsigned int sizeInBytes;
        };

        /*struct BitReservoir {
            std::vector<uint8_t> data;
            unsigned int startingFrameIndex;
            unsigned int positionInBytes;
        };*/

        bool synchronizeToNextFrame(std::istream &inputStream, std::array<uint8_t, Frame::Header::headerSize> &headerData, uint8_t const versionMask, uint8_t const versionValue) const;
        Frame::Header getFrameHeaderAtIndex(std::istream &inputStream, unsigned int const frameIndex);
        bool moveToFrameAtIndex(std::istream &inputStream, unsigned int const index);
        std::vector<uint8_t> getSideInformationData(std::istream &inputStream, Frame::Header const &header) const;

        std::vector<FrameEntry> _frameEntries;
        //BitReservoir _bitReservoir;//TODO: voir si besoin
        std::array<std::array<float, 576>, 2> _framesBlocksSubbandsOverlappingValues;
        std::array<std::array<float, 1024>, 2> _framesShiftedAndMatrixedSubbandsValues;
    };

    #include "Decoder_s.hpp"

}

#endif /* MP3_DECODER_HPP */
