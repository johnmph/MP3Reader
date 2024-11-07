#ifndef MP3_DECODER_HPP
#define MP3_DECODER_HPP

#include <cstdint>
#include <istream>
#include <vector>
#include <unordered_set>
#include <optional>
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

        std::optional<std::vector<uint8_t>> getFrameDataFromBitReservoir(std::istream &inputStream, unsigned int const frameIndex, Frame::SideInformation const &frameSideInformation);
        uint16_t getCRCIfExist(std::istream &inputStream, Frame::Header const &frameHeader);
        uint16_t calculateCRC(std::array<uint8_t, 4> const &headerData, std::vector<uint8_t> const &sideInformationData);
        std::optional<Frame::Header> tryToGetFrameHeaderAtIndex(std::istream &inputStream, unsigned int const frameIndex);
        std::optional<std::array<uint8_t, Frame::Header::headerSize>> tryToReadNextFrameHeaderData(std::istream &inputStream) const;

        uint8_t const _versionMask;
        uint8_t const _versionValue;
        std::vector<FrameEntry> _frameEntries;
        //BitReservoir _bitReservoir;//TODO: voir si besoin
        std::array<std::array<float, 576>, 2> _framesBlocksSubbandsOverlappingValues;
        std::array<std::array<float, 1024>, 2> _framesShiftedAndMatrixedSubbandsValues;
    };

    struct Exception : std::exception {
        Exception(Decoder &decoder) :_decoder(decoder) {}

    private:
        Decoder &_decoder;
    };

    struct FrameNotFound : Exception {
        FrameNotFound(Decoder &decoder, unsigned int const index) : Exception(decoder), _index(index) {}

    private:
        unsigned int const _index;
    };

    struct FrameCRCIncorrect : Exception {
        FrameCRCIncorrect(Decoder &decoder, uint16_t crcStored, uint16_t crcCalculated) : Exception(decoder), _crcStored(crcStored), _crcCalculated(crcCalculated) {}

    private:
        //Frame::Header &_header;
        uint16_t const _crcStored;
        uint16_t const _crcCalculated;
    };

    #include "Decoder_s.hpp"

}

#endif /* MP3_DECODER_HPP */
