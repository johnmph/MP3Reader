
#include <iostream>
#include <vector>
#include <fstream>
#include <portaudio.h>

#include "MP3/Decoder.hpp"


#define SAMPLE_RATE   (44100)


struct PortAudio {//TODO:renommer
    PortAudio(MP3::Decoder &decoder, std::istream &inputStream) : _result(Pa_Initialize()), _stream(nullptr), _decoder(decoder), _inputStream(inputStream), _currentFrameIndex(0), _currentPositionInFrame(0), _isPlaying(false) {
        _currentFramePCMValues.resize(/*4*/2);//TODO: selon le nombre de canaux !

        _numberOfFrames = _decoder.getNumberOfFrames(_inputStream);

        std::cout << "Is valid MP3 = " << (_decoder.isValidFormat(_inputStream, 3) ? "YES" : "NO") << "\n";
        std::cout << "Number of frames = " << _numberOfFrames << "\n";
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

        outputParameters.channelCount = /*2*/1;       /* stereo output */
        outputParameters.sampleFormat = paFloat32; /* 32 bit float output */
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
                auto frame = _decoder.getFrameAtIndex(_inputStream, _currentFrameIndex);
                for (unsigned int i = 0; i < /*4*/2; ++i) {
                    _currentFramePCMValues[i] = frame.getPCMSamples(/*i / 2, i % 2*/i, 0);
                }

                // Go to next frame
                ++_currentFrameIndex;
            }

            unsigned int granuleIndex = (_currentPositionInFrame > 575) ? 1 : 0;
            *out++ = _currentFramePCMValues[/*(granuleIndex * 2) + 0*/granuleIndex][_currentPositionInFrame - (576 * granuleIndex)];
            //*out++ = _currentFramePCMValues[(granuleIndex * 2) + 1][_currentPositionInFrame - (576 * granuleIndex)];

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
    MP3::Decoder _decoder;
    std::istream &_inputStream;
    unsigned int _currentFrameIndex;
    unsigned int _numberOfFrames;
    unsigned int _currentPositionInFrame;
    std::vector<std::array<float, 576>> _currentFramePCMValues;
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

    std::ifstream mp3Stream("Mono_cbr.mp3", std::ios::binary);

    MP3::Decoder mp3Decoder;

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
