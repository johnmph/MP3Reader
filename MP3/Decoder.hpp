#ifndef MP3_DECODER_HPP
#define MP3_DECODER_HPP

#include <cstdint>
#include <istream>
#include <vector>
#include <unordered_set>
#include <optional>
#include <memory>
#include "Helper.hpp"
#include "Frame/Header.hpp"
#include "Frame/Frame.hpp"


namespace MP3 {
    
    struct Decoder {

        static constexpr uint8_t versionMask = 0xFE;
        static constexpr uint8_t versionValue = 0xFA;
        
        static bool isValidFormat(std::istream &inputStream, unsigned int const numberOfFramesForValidFormat);

        Decoder(std::unique_ptr<std::istream> inputStream, unsigned int const numberOfFramesForValidFormat);

        template <class TFunction>
        void browseFramesHeader(TFunction &&browseFunc);

        unsigned int getNumberOfFrames();
        std::unordered_set<unsigned int> const &getBitrates();
        std::unordered_set<unsigned int> const &getSamplingRates();

        template <class TFunction>
        Frame::Frame getFrameAtIndex(unsigned int const frameIndex, TFunction &&errorFunction);
        
    private:

        static constexpr uint16_t crcPolynomial = 0x8005;
        static constexpr uint16_t crcInitialValue = 0xFFFF;


        struct FrameEntry {//TODO: on peut avoir aussi une version qui sauve les frame directement plutot que la position dans le fichier ainsi on garde les données en mémoire, et on peut choisir ce que l'on veut via un template parameter ou un truc dans le genre
            unsigned int positionInBytes;
            unsigned int sizeInBytes;
        };


        static std::optional<FrameEntry> findFirstValidFrame(std::istream &inputStream, unsigned int const numberOfFramesForValidFormat);
        static std::optional<std::array<uint8_t, Frame::Header::headerSize>> tryToReadNextFrameHeaderData(std::istream &inputStream);

        bool getFrameDataFromBitReservoir(unsigned int const frameIndex, Frame::SideInformation const &frameSideInformation, std::vector<uint8_t> &frameData);
        uint16_t getCRCIfExist(Frame::Header const &frameHeader);
        uint16_t calculateCRC(std::array<uint8_t, 4> const &headerData, std::vector<uint8_t> const &sideInformationData);
        std::optional<Frame::Header> tryToGetFrameHeaderAtIndex(unsigned int const frameIndex);

        std::unique_ptr<std::istream> _inputStream;
        std::vector<FrameEntry> _frameEntries;
        std::array<std::array<float, 576>, 2> _framesBlocksSubbandsOverlappingValues;
        std::array<std::array<float, 1024>, 2> _framesShiftedAndMatrixedSubbandsValues;
        std::unordered_set<unsigned int> _bitrates;
        std::unordered_set<unsigned int> _samplingRates;
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
