
#include <iostream>
#include <vector>
#include <fstream>
#include <portaudio.h>

#include "MP3Decoder.hpp"


#define SAMPLE_RATE   (44100)


struct PortAudio {//TODO:renommer
    PortAudio(std::vector<uint16_t> &&data) : _result(Pa_Initialize()), _stream(nullptr), _data(std::move(data)), _currentPosition(0), _isPlaying(false) {
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
        outputParameters.sampleFormat = paInt16; /* 16 bit integer output */
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = nullptr;

        PaError err = Pa_OpenStream(&_stream, nullptr, &outputParameters, SAMPLE_RATE, paFramesPerBufferUnspecified, paClipOff, &PortAudio::streamRequestCallback, this);
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
        uint16_t *out = static_cast<uint16_t *>(outputBuffer);

        (void) timeInfo; /* Prevent unused variable warnings. */
        (void) statusFlags;
        (void) inputBuffer;

        for (unsigned long i = 0; (i < framesPerBuffer) && (_currentPosition < _data.size()); i++) {
            *out++ = _data[_currentPosition++];
            *out++ = _data[_currentPosition++];
        }

        return (_currentPosition < _data.size()) ? paContinue : paComplete;
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
    std::vector<uint16_t> _data;
    int _currentPosition;
    bool _isPlaying;
};

int main(void) {/*
    std::ifstream ifs("01.wav", std::ios::binary);

    ifs.clear();
    ifs.seekg(0x28);

    uint32_t dataSize = 0;
    ifs.read(reinterpret_cast<char *>(&dataSize), sizeof(dataSize));

    std::vector<uint16_t> data(dataSize / 2);
    ifs.read(reinterpret_cast<char *>(data.data()), dataSize);

    PortAudio portAudio(std::move(data));

    if (portAudio.open(Pa_GetDefaultOutputDevice())) {
        if (portAudio.start()) {

            for (; portAudio.isPlaying();) {
                Pa_Sleep(1000);
            }

            portAudio.stop();
        }

        portAudio.close();
    }

    printf("Test finished.\n");
    return paNoError;

error:
    fprintf( stderr, "An error occurred while using the portaudio stream\n" );
    return 1;*/

    std::ifstream mp3Stream("01.mp3", std::ios::binary);

    mp3Stream.clear();
    mp3Stream.seekg(0);

    MP3::Decoder mp3Decoder;

    int size = 0;
    while ((mp3Stream.eof() == false) && (mp3Stream.good())) {
        auto frameHeader = mp3Decoder.getNextFrame(mp3Stream);
        int i = mp3Decoder.getFrameSize(frameHeader);
        size += i;

        mp3Stream.seekg(i - 4, std::ios_base::cur);

        std::cout << (mp3Stream.tellg() / 10000) << "\n";
    }

    std::cout << size << "\n";
}
