#ifndef MP3_DECODER_HPP
#define MP3_DECODER_HPP

#include <cstdint>
#include <istream>
#include <vector>
#include <unordered_set>
#include "Frame/Header.hpp"
#include "Frame/Frame.hpp"


namespace MP3 {
    
    //template <class TStream>//TODO: si on veut le stream géré ici
    struct Decoder {

        // TODO: creer des constantes pour le versionMask et versionValue

        // TODO: avoir un constructor avec plusieurs parametres (ou une struct Configuration) et avoir par exemple le nombre de frames a lire pour etre certain que c'est un bon fichier MP3
        Decoder(uint8_t const versionMask, uint8_t const versionValue);

        template <class TFunction>
        void browseFramesHeader(std::istream &inputStream, TFunction &&browseFunc);

        bool isValidFormat(std::istream &inputStream, unsigned int const numberOfFramesForValidFormat);//TODO: (dans le cas ou on passe le stream en move avec le template parameter sur le type de stream) a mettre en static et a appeler dans le constructor et faire une exception si retourne false dans le constructor (comme dans la classe Header)
        unsigned int getNumberOfFrames(std::istream &inputStream);
        std::unordered_set<unsigned int> getBitrates(std::istream &inputStream);    // TODO: lazy creation et donc avoir un membre _bitrates et pareil pour la methode en dessous
        std::unordered_set<unsigned int> getSamplingRates(std::istream &inputStream);

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

        bool tryToReadNextFrameHeaderData(std::istream &inputStream, std::array<uint8_t, Frame::Header::headerSize> &headerData) const;
        Frame::Header getFrameHeaderAtIndex(std::istream &inputStream, unsigned int const frameIndex);
        bool moveToFrameAtIndex(std::istream &inputStream, unsigned int const index);
        Frame::SideInformation getFrameSideInformation(std::istream &inputStream, Frame::Header const &frameHeader) const;

        uint8_t const _versionMask;
        uint8_t const _versionValue;
        std::vector<FrameEntry> _frameEntries;
        //BitReservoir _bitReservoir;//TODO: voir si besoin
        std::array<std::array<float, 576>, 2> _framesBlocksSubbandsOverlappingValues;
        std::array<std::array<float, 1024>, 2> _framesShiftedAndMatrixedSubbandsValues;
    };

    #include "Decoder_s.hpp"

}

#endif /* MP3_DECODER_HPP */
