#ifndef MP3_DECODER_HPP
#define MP3_DECODER_HPP

#include <cstdint>
#include <istream>
#include <vector>
#include <unordered_set>
#include <optional>
#include "Helper.hpp"
#include "Frame/Header.hpp"
#include "Frame/Frame.hpp"


namespace MP3 {
    
    //template <class TStream>//TODO: si on veut le stream géré ici
    struct Decoder {

        // TODO: creer des constantes pour le versionMask et versionValue

        // TODO: avoir un constructor avec plusieurs parametres (ou une struct Configuration) et avoir par exemple le nombre de frames a lire pour etre certain que c'est un bon fichier MP3
        Decoder(std::istream &inputStream, uint8_t const versionMask, uint8_t const versionValue, unsigned int const numberOfFramesForValidFormat);

        template <class TFunction>
        void browseFramesHeader(std::istream &inputStream, TFunction &&browseFunc);

        //bool isValidFormat(std::istream &inputStream, unsigned int const numberOfFramesForValidFormat);//TODO: (dans le cas ou on passe le stream en move avec le template parameter sur le type de stream) a mettre en static et a appeler dans le constructor et faire une exception si retourne false dans le constructor (comme dans la classe Header)
        unsigned int getNumberOfFrames(std::istream &inputStream);
        std::unordered_set<unsigned int> getBitrates(std::istream &inputStream);    // TODO: lazy creation et donc avoir un membre _bitrates et pareil pour la methode en dessous
        std::unordered_set<unsigned int> getSamplingRates(std::istream &inputStream);

        template <class TFunction>
        Frame::Frame getFrameAtIndex(std::istream &inputStream, unsigned int const frameIndex, TFunction &&errorFunction);
        
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

        bool checkFormat(std::istream &inputStream, unsigned int const numberOfFramesForValidFormat);
        bool getFrameDataFromBitReservoir(std::istream &inputStream, unsigned int const frameIndex, Frame::SideInformation const &frameSideInformation, std::vector<uint8_t> &frameData);
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


    namespace Error {

        struct DecoderException : std::exception {
            DecoderException(Decoder &decoder);

        private:
            Decoder &_decoder;
        };

        struct BadMP3Format : DecoderException {
            BadMP3Format(Decoder &decoder, std::istream &inputStream);

            std::istream &getInputStream() const;

        private:
            std::istream &_inputStream;
        };

        struct FrameDecoderException : DecoderException {
            FrameDecoderException(Decoder &decoder, unsigned int const frameIndex);

            unsigned int getFrameIndex() const;

        private:
            unsigned int const _frameIndex;
        };

        struct FrameNotFound : FrameDecoderException {
            FrameNotFound(Decoder &decoder, unsigned int const frameIndex);
        };

        struct FrameCRCIncorrect : FrameDecoderException {
            FrameCRCIncorrect(Decoder &decoder, unsigned int const frameIndex, uint16_t crcStored, uint16_t crcCalculated, Frame::Header &frameHeader, std::array<uint8_t, Frame::Header::headerSize> const &frameHeaderData, std::vector<uint8_t> &frameSideInformationData);
            FrameCRCIncorrect(Decoder &decoder, unsigned int const frameIndex, uint16_t crcStored, uint16_t crcCalculated, Frame::Header const &frameHeader, std::array<uint8_t, Frame::Header::headerSize> const &frameHeaderData, std::vector<uint8_t> const &frameSideInformationData);

            uint16_t getCRCStored() const;
            uint16_t getCRCCalculated() const;
            Frame::Header &getFrameHeader() const;
            std::array<uint8_t, Frame::Header::headerSize> const &getFrameHeaderData() const;
            std::vector<uint8_t> &getFrameSideInformationData() const;

        private:
            uint16_t const _crcStored;
            uint16_t const _crcCalculated;
            Frame::Header _frameHeaderTemp;
            Frame::Header &_frameHeader;
            std::array<uint8_t, Frame::Header::headerSize> const _frameHeaderData;//TODO: pas une reference pour eviter le probleme des references invalides au throw
            std::vector<uint8_t> _frameSideInformationDataTemp;
            std::vector<uint8_t> &_frameSideInformationData;
        };

        struct FrameNoData : FrameDecoderException {
            FrameNoData(Decoder &decoder, unsigned int const frameIndex, std::vector<uint8_t> &frameData);
            FrameNoData(Decoder &decoder, unsigned int const frameIndex, std::vector<uint8_t> const &frameData);

            std::vector<uint8_t> &getFrameData() const;

        private:
            std::vector<uint8_t> _frameDataTemp;
            std::vector<uint8_t> &_frameData;
        };
        
    }


    #include "Decoder_s.hpp"

}

#endif /* MP3_DECODER_HPP */
