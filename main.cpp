
#include <iostream>
#include <vector>
#include <fstream>
#include <optional>
#include <portaudio.h>

#include "MP3/Decoder.hpp"


struct PortAudio {//TODO:renommer
    PortAudio(MP3::Decoder &decoder, std::istream &inputStream) : _result(Pa_Initialize()), _stream(nullptr), _decoder(decoder), _inputStream(inputStream), _currentFrameIndex(0), _currentPositionInFrame(0), _isPlaying(false) {
        _numberOfFrames = _decoder.getNumberOfFrames(_inputStream);

        auto const bitrates = _decoder.getBitrates(_inputStream);
        auto const samplingRates = _decoder.getSamplingRates(_inputStream);

        std::cout << "Is valid MP3 = " << (_decoder.isValidFormat(_inputStream, 3) ? "YES" : "NO") << "\n";
        std::cout << "Number of frames = " << _numberOfFrames << "\n";
        std::cout << "Is VBR : " << ((bitrates.size() > 1) ? "YES" : "NO") << "\n";
        std::cout << "Has multiple sampling rates : " << ((samplingRates.size() > 1) ? "YES" : "NO") << "\n";

        if (bitrates.size() == 1) {
            std::cout << "Bitrate = " << *(std::begin(bitrates)) << "\n";//TODO: trouver tous les .begin / .end et remplacer par std::begin, std::end
        }

        if (samplingRates.size() == 1) {
            std::cout << "Sampling rate = " << *(std::begin(samplingRates)) << "\n";
            _samplingRate = *(std::begin(samplingRates));
        } else {
            _samplingRate = 44100;
        }
    }

    ~PortAudio() {
        if (_result != paNoError) {
            return;
        }

        Pa_Terminate();
    }

    bool open(PaDeviceIndex index) {
        PaStreamParameters outputParameters;

        outputParameters.device = index;
        if (outputParameters.device == paNoDevice) {
            return false;
        }

        PaDeviceInfo const *pInfo = Pa_GetDeviceInfo(index);
        if (pInfo != nullptr) {
            std::cout << "Output device name : " << pInfo->name << "\n";
        }

        outputParameters.channelCount = 2;       /* stereo output */
        outputParameters.sampleFormat = paFloat32; /* 32 bit float output */
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = nullptr;

        PaError err = Pa_OpenStream(&_stream, nullptr, &outputParameters, _samplingRate, paFramesPerBufferUnspecified, paClipOff, &PortAudio::streamRequestCallback, this);
        if (err != paNoError) {
            /* Failed to open stream to device !!! */
            return false;
        }

        err = Pa_SetStreamFinishedCallback(_stream, &PortAudio::streamFinishedCallback);

        if (err != paNoError) {
            Pa_CloseStream(_stream);
            _stream = nullptr;

            return false;
        }

        return true;
    }

    bool close() {
        if (_stream == nullptr) {
            return false;
        }
        
        PaError err = Pa_CloseStream(_stream);
        _stream = nullptr;

        return (err == paNoError);
    }

    bool start() {
        if (_stream == nullptr) {
            return false;
        }

        PaError err = Pa_StartStream(_stream);

        _isPlaying = true;

        return (err == paNoError);
    }

    bool stop() {
        if (_stream == nullptr) {
            return false;
        }

        PaError err = Pa_StopStream(_stream);

        return (err == paNoError);
    }

    bool isPlaying() {
        return _isPlaying;
    }

private:
    /* The instance callback, where we have access to every method/variable in object of class Sine */
    int paCallbackMethod(void const *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, PaStreamCallbackTimeInfo const *timeInfo, PaStreamCallbackFlags statusFlags) {
        float *out = static_cast<float *>(outputBuffer);

        (void) timeInfo; /* Prevent unused variable warnings. */
        (void) statusFlags;
        (void) inputBuffer;

        // Fill buffer
        for (unsigned int i = 0; (i < framesPerBuffer) && (_currentFrameIndex < _numberOfFrames); i++) {
            // Get a new frame if necessary
            if (_currentPositionInFrame == 0) {
                // Check if stream is good
                if (_inputStream.good() == false) {
                    return paAbort;
                }

                // Get current frame
                _currentFrame.emplace(_decoder.getFrameAtIndex(_inputStream, _currentFrameIndex));

                // Get number of channels
                _currentFrameNumberOfChannels = (*_currentFrame).getNumberOfChannels();

                // Go to next frame
                ++_currentFrameIndex;
            }

            // Copy value to audio buffer
            *out++ = (*_currentFrame).getPCMSamples(0)[_currentPositionInFrame];
            *out++ = (*_currentFrame).getPCMSamples(_currentFrameNumberOfChannels - 1)[_currentPositionInFrame];

            // Check position in frame
            ++_currentPositionInFrame;

            if (_currentPositionInFrame > 1151) {
                _currentPositionInFrame = 0;
            }
        }

        // Check if continue or not
        return (_currentFrameIndex < _numberOfFrames) ? paContinue : paComplete;
    }

    void paStreamFinishedMethod() {
        _isPlaying = false;
    }

    /* This routine will be called by the PortAudio engine when audio is needed.
    ** It may called at interrupt level on some machines so don't do anything
    ** that could mess up the system like calling malloc() or free().
    */
    static int streamRequestCallback(void const *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, PaStreamCallbackTimeInfo const *timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
        /* Here we cast userData to Sine* type so we can call the instance method paCallbackMethod, we can do that since
           we called Pa_OpenStream with 'this' for userData */
        return static_cast<PortAudio *>(userData)->paCallbackMethod(inputBuffer, outputBuffer, framesPerBuffer, timeInfo, statusFlags);
    }

    /*
     * This routine is called by portaudio when playback is done.
     */
    static void streamFinishedCallback(void* userData) {
        return static_cast<PortAudio *>(userData)->paStreamFinishedMethod();
    }

    PaError _result;
    PaStream *_stream;
    MP3::Decoder _decoder;//TODO: par copie ???
    std::istream &_inputStream;
    std::optional<MP3::Frame::Frame> _currentFrame;
    unsigned int _currentFrameIndex;
    unsigned int _numberOfFrames;
    unsigned int _currentPositionInFrame;
    unsigned int _currentFrameNumberOfChannels;
    bool _isPlaying;
    unsigned int _samplingRate;
};

int main(void) {
    std::ifstream mp3Stream("Mono_cbr2.mp3", std::ios::binary);

    MP3::Decoder mp3Decoder(0xFE, 0xFA);

    PortAudio portAudio(mp3Decoder, mp3Stream);

    if (portAudio.open(Pa_GetDefaultOutputDevice())) {
        if (portAudio.start()) {

            for (; portAudio.isPlaying();) {
                Pa_Sleep(1000);
            }

            portAudio.stop();
        }

        portAudio.close();
    }
}
