#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <portaudio.h>
#include <time.h>

#define SAMPLE_RATE 44100
#define PI 3.14159265358979323846
#define TOTAL_KEYS 36
#define DELAY_BUFFER_SIZE 44100

typedef struct {
    int key;
    double frequency;
} Note;

bool isPlaying = false;
bool stopPlayback = false;
int activeKeys[TOTAL_KEYS];
int numActiveKeys = 0;
int currentPlayingKey = -1;
size_t noteIndex = 0;
double phase = 0.0;
float delayBuffer[DELAY_BUFFER_SIZE] = {0};
int delayIndex = 0;
int waveformType = 1;
bool enableDelay = false;

time_t lastSwitchTime;
double quantumTime = 1.0;

bool noteIsSilent = false;
int silenceSamplesRemaining = 0;

const double keyFrequencies[TOTAL_KEYS] = {
    65.41, 73.42, 82.41, 87.31, 98.00, 110.00, 123.47,
    130.81, 146.83, 164.81, 174.61, 196.00, 220.00, 246.94,
    261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88,
    523.25, 587.33, 659.25, 698.46, 783.99, 880.00, 987.77,
    1046.50, 1174.66, 1318.51, 1396.91, 1567.98, 1760.00, 1975.53, 2093.00
};

float generateWave(double phase) {
    switch (waveformType) {
        case 1: return sin(phase);
        case 2: return (2.0 * (phase / (2 * PI)) - 1.0);
        case 3: return (sin(phase) >= 0 ? 1.0 : -1.0);
        case 4: return fabs(fmod(phase, 2 * PI) - PI) / PI;
        default: return sin(phase);
    }
}

float applyDelay(float sample) {
    float delayedSample = delayBuffer[delayIndex];
    delayBuffer[delayIndex] = sample + delayedSample * 0.5;
    delayIndex = (delayIndex + 1) % DELAY_BUFFER_SIZE;
    return enableDelay ? delayedSample : sample;
}

float applyLimiter(float sample, float threshold) {
    if (sample > threshold) return threshold;
    if (sample < -threshold) return -threshold;
    return sample;
}

static int audioCallback(const void *input, void *output,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags, void *userData) {
    float *out = (float *)output;
    if (numActiveKeys == 0 || stopPlayback) {
        for (unsigned int i = 0; i < framesPerBuffer; i++) {
            *out++ = 0.0f;
            *out++ = 0.0f;
        }
        return paContinue;
    }

    time_t currentTime = time(NULL);
    if (!noteIsSilent && difftime(currentTime, lastSwitchTime) >= quantumTime) {
        noteIndex = (noteIndex + 1) % numActiveKeys;
        lastSwitchTime = currentTime;
        noteIsSilent = true;
        silenceSamplesRemaining = SAMPLE_RATE * 0.1; 
        phase = 0.0; 
    }

    if (!noteIsSilent)
        currentPlayingKey = activeKeys[noteIndex];
    else
        currentPlayingKey = -1;

    double frequency = keyFrequencies[activeKeys[noteIndex] - 1];
    double phaseIncrement = (2.0 * PI * frequency) / SAMPLE_RATE;

    for (unsigned int i = 0; i < framesPerBuffer; i++) {
        float sample = 0.0f;
        if (noteIsSilent) {
            sample = 0.0f;
            silenceSamplesRemaining--;
            if (silenceSamplesRemaining <= 0) {
                noteIsSilent = false;
            }
        } else {
            sample = generateWave(phase);
            sample = applyDelay(sample);
            phase += phaseIncrement;
            if (phase >= (2.0 * PI)) phase -= (2.0 * PI);
        }

        sample = applyLimiter(sample, 0.9f);

        *out++ = sample;
        *out++ = sample;
    }

    return paContinue;
}

void *playNotes(void *args) {
    Pa_Initialize();
    PaStream *stream;
    Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, SAMPLE_RATE, 256, audioCallback, NULL);
    Pa_StartStream(stream);
    lastSwitchTime = time(NULL);
    
    while (isPlaying) {
        SDL_Delay(100);
    }
    
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    return NULL;
}

void renderUI(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);
    
    int screenWidth = 1600;
    int screenHeight = 600;
    int width = screenWidth, height = screenHeight;
    int whiteKeyWidth = width / 36;
    int blackKeyWidth = whiteKeyWidth * 0.6;
    int blackKeyHeight = height * 0.6;
    
    for (int i = 0, keyNumber = 1; i < 36; i++, keyNumber++) {
        SDL_Rect rect = { i * whiteKeyWidth, 0, whiteKeyWidth - 2, height };
        SDL_SetRenderDrawColor(renderer, (keyNumber == currentPlayingKey) ? 180 : 245, 245, 245, 255);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
    
    int blackKeyPositions[] = {1, 3, 6, 8, 10, 13, 15, 18, 20, 22, 25, 27, 30, 32, 34};
    for (int i = 0; i < 15; i++) {
        SDL_Rect rect = { (blackKeyPositions[i] * whiteKeyWidth) - (blackKeyWidth / 2), 0, blackKeyWidth, blackKeyHeight };
        SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
        SDL_RenderFillRect(renderer, &rect);
    }
    SDL_RenderPresent(renderer);
}

void *runGUI(void *args) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Synthwave Piano", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1600, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Event event;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        renderUI(renderer);
        SDL_Delay(16);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return NULL;
}

int SDL_main(int argc, char *argv[]) {
    printf("Enter number of keys: ");
    scanf("%d", &numActiveKeys);
    printf("Enter keys (1-36 for notes C2 to C7): ");
    for (int i = 0; i < numActiveKeys; i++) {
        scanf("%d", &activeKeys[i]);
    }
    printf("Enter quantum time (seconds per note): ");
    scanf("%lf", &quantumTime);
    printf("Select waveform type (1: Sine, 2: Sawtooth, 3: Square, 4: Triangle): ");
    scanf("%d", &waveformType);
    
    isPlaying = true;
    pthread_t audioThread, guiThread;
    pthread_create(&audioThread, NULL, playNotes, NULL);
    pthread_create(&guiThread, NULL, runGUI, NULL);
    pthread_join(guiThread, NULL);
    isPlaying = false;
    pthread_join(audioThread, NULL);
    return 0;
}
