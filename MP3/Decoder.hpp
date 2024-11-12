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
#include <iostream>//TODO: a retirer

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

        struct Exception : std::exception {
            Exception(Decoder &decoder) :_decoder(decoder) {}

        private:
            Decoder &_decoder;
        };

        struct FrameException : Exception {
            FrameException(Decoder &decoder, unsigned int const frameIndex) : Exception(decoder), _frameIndex(frameIndex) {}

            unsigned int getFrameIndex() const {
                return _frameIndex;
            }

        private:
            unsigned int const _frameIndex;
        };

        struct FrameNotFound : FrameException {
            FrameNotFound(Decoder &decoder, unsigned int const frameIndex) : FrameException(decoder, frameIndex) {}
        };

        struct FrameCRCIncorrect : FrameException {
            FrameCRCIncorrect(Decoder &decoder, unsigned int const frameIndex, uint16_t crcStored, uint16_t crcCalculated, Frame::Header &frameHeader, std::array<uint8_t, Frame::Header::headerSize> const &frameHeaderData, std::vector<uint8_t> &frameSideInformationData) : FrameException(decoder, frameIndex), _crcStored(crcStored), _crcCalculated(crcCalculated), _frameHeaderTemp(frameHeader), _frameHeader(frameHeader), _frameHeaderData(frameHeaderData), _frameSideInformationData(frameSideInformationData) {}
            FrameCRCIncorrect(Decoder &decoder, unsigned int const frameIndex, uint16_t crcStored, uint16_t crcCalculated, Frame::Header const &frameHeader, std::array<uint8_t, Frame::Header::headerSize> const &frameHeaderData, std::vector<uint8_t> const &frameSideInformationData) : FrameException(decoder, frameIndex), _crcStored(crcStored), _crcCalculated(crcCalculated), _frameHeaderTemp(frameHeader), _frameHeader(_frameHeaderTemp), _frameHeaderData(frameHeaderData), _frameSideInformationDataTemp(frameSideInformationData), _frameSideInformationData(_frameSideInformationDataTemp) {}

            uint16_t getCRCStored() const {
                return _crcStored;
            }

            uint16_t getCRCCalculated() const {
                return _crcCalculated;
            }

            Frame::Header &getFrameHeader() const {
                return _frameHeader;
            }

            std::array<uint8_t, Frame::Header::headerSize> const &getFrameHeaderData() const {
                return _frameHeaderData;
            }

            std::vector<uint8_t> &getFrameSideInformationData() const {
                return _frameSideInformationData;
            }

        private:
            uint16_t const _crcStored;
            uint16_t const _crcCalculated;
            Frame::Header _frameHeaderTemp;
            Frame::Header &_frameHeader;
            std::array<uint8_t, Frame::Header::headerSize> const _frameHeaderData;//TODO: pas une reference pour eviter le probleme des references invalides au throw
            std::vector<uint8_t> _frameSideInformationDataTemp;
            std::vector<uint8_t> &_frameSideInformationData;
        };

        struct FrameNoData : FrameException {
            FrameNoData(Decoder &decoder, unsigned int const frameIndex, std::vector<uint8_t> &frameData) : FrameException(decoder, frameIndex), _frameData(frameData) {}
            FrameNoData(Decoder &decoder, unsigned int const frameIndex, std::vector<uint8_t> const &frameData) : FrameException(decoder, frameIndex), _frameDataTemp(frameData), _frameData(_frameDataTemp) {}

            std::vector<uint8_t> &getFrameData() const {
                return _frameData;
            }

        private:
            std::vector<uint8_t> _frameDataTemp;
            std::vector<uint8_t> &_frameData;
        };
        
    }


    #include "Decoder_s.hpp"

}

#endif /* MP3_DECODER_HPP */
