#include "raylib.h"
#include "synth.h"
#include "effects.h"
#include "midi.h"
#include "ui.h"
#include <stdio.h>
#include <pthread.h>

// Physical display dimensions (portrait WaveShare panel)
#define PHYSICAL_WIDTH  400
#define PHYSICAL_HEIGHT 1280

// Global state (accessible from audio callback)
static Synth g_synth;
static Effects g_effects;
static UI g_ui;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static AudioStream g_stream;
static const int BUFFER_SIZES[] = {512, 256, 128};

// Audio callback - called by raylib to fill audio buffer
static void SynthAudioCallback(void *buffer, unsigned int frames) {
    float *out = (float *)buffer;

    pthread_mutex_lock(&g_mutex);

    for (unsigned int i = 0; i < frames; i++) {
        // Generate synth sample
        float sample = synth_process(&g_synth);

        // Apply effects
        sample = effects_process(&g_effects, sample);

        // Clamp output
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;

        // Stereo output
        out[i * 2] = sample;
        out[i * 2 + 1] = sample;

        // Feed to waveform display (every few samples to avoid too much overhead)
        ui_add_sample(&g_ui, sample);
    }

    pthread_mutex_unlock(&g_mutex);
}

// Handle MIDI CC messages
static void handle_midi_cc(int cc, int value) {
    float normalized = (float)value / 127.0f;

    switch (cc) {
        case CC_FILTER_CUTOFF:
            synth_set_filter(&g_synth, normalized,
                           g_synth.filter_resonance, g_synth.filter_type);
            break;

        case CC_FILTER_RESO:
            synth_set_filter(&g_synth, g_synth.filter_cutoff,
                           normalized * 0.95f, g_synth.filter_type);
            break;

        case CC_ATTACK:
            g_synth.attack = 0.001f + normalized * 2.0f;
            break;

        case CC_RELEASE:
            g_synth.release = 0.001f + normalized * 3.0f;
            break;

        case CC_REVERB:
            reverb_set_mix(&g_effects.reverb, normalized);
            break;

        case CC_DELAY:
            delay_set_mix(&g_effects.delay, normalized);
            break;

        case CC_MOD_WHEEL:
            // Map mod wheel to filter cutoff modulation
            synth_set_filter(&g_synth, normalized,
                           g_synth.filter_resonance, g_synth.filter_type);
            break;
    }
}

int main(void) {
    // Initialize raylib with physical display dimensions (portrait)
    InitWindow(PHYSICAL_WIDTH, PHYSICAL_HEIGHT, "ButterySynth");
    SetTargetFPS(60);

    // Create render texture for logical landscape content
    RenderTexture2D target = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Initialize audio
    InitAudioDevice();

    // Initialize synth components BEFORE starting audio stream
    synth_init(&g_synth);
    effects_init(&g_effects);

    // Initialize UI (needs synth/effects pointers)
    ui_init(&g_ui, &g_synth, &g_effects);

    // Create audio stream with initial buffer size from UI
    SetAudioStreamBufferSizeDefault(BUFFER_SIZES[g_ui.buffer_size]);
    g_stream = LoadAudioStream(44100, 32, 2);
    SetAudioStreamCallback(g_stream, SynthAudioCallback);
    PlayAudioStream(g_stream);

    // Initialize MIDI
    MidiInput midi;
    int midi_ok = midi_init(&midi);
    if (midi_ok < 0) {
        printf("Warning: MIDI initialization failed. Continuing without MIDI.\n");
    }

    printf("ButterySynth started!\n");
    printf("  - Display: %dx%d physical -> %dx%d logical\n",
           PHYSICAL_WIDTH, PHYSICAL_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);
    printf("  - Audio: 44100Hz stereo\n");
    printf("  - Voices: %d\n", NUM_VOICES);
    printf("  - Touch: %s\n", GetTouchPointCount() > 0 ? "Yes" : "No");

    // Main loop
    while (!WindowShouldClose()) {
        // Poll MIDI events
        if (midi_ok >= 0) {
            MidiEvent event;
            while (midi_poll(&midi, &event)) {
                pthread_mutex_lock(&g_mutex);

                switch (event.type) {
                    case MIDI_NOTE_ON:
                        if (event.data2 > 0) {
                            synth_note_on(&g_synth, event.data1, event.data2);
                        } else {
                            synth_note_off(&g_synth, event.data1);
                        }
                        break;

                    case MIDI_NOTE_OFF:
                        synth_note_off(&g_synth, event.data1);
                        break;

                    case MIDI_CONTROL:
                        handle_midi_cc(event.data1, event.data2);
                        break;
                }

                pthread_mutex_unlock(&g_mutex);
            }
        }

        // Update UI (handles touch input)
        pthread_mutex_lock(&g_mutex);
        ui_update(&g_ui);

        // Handle panic button
        if (g_ui.panic_triggered) {
            synth_panic(&g_synth);
            g_ui.panic_triggered = false;
        }

        // Handle buffer size change
        if (g_ui.buffer_changed) {
            StopAudioStream(g_stream);
            UnloadAudioStream(g_stream);
            SetAudioStreamBufferSizeDefault(BUFFER_SIZES[g_ui.buffer_size]);
            g_stream = LoadAudioStream(44100, 32, 2);
            SetAudioStreamCallback(g_stream, SynthAudioCallback);
            PlayAudioStream(g_stream);
            g_ui.buffer_changed = false;
            printf("Audio buffer changed to %d samples\n", BUFFER_SIZES[g_ui.buffer_size]);
        }

        pthread_mutex_unlock(&g_mutex);

        // Draw UI to render texture (logical landscape coordinates)
        BeginTextureMode(target);
        pthread_mutex_lock(&g_mutex);
        ui_draw(&g_ui);
        pthread_mutex_unlock(&g_mutex);
        EndTextureMode();

        // Draw rotated texture to physical screen
        BeginDrawing();
        ClearBackground(BLACK);
        Rectangle source = { 0, 0, (float)SCREEN_WIDTH, -(float)SCREEN_HEIGHT };
        Rectangle dest = { 0, (float)PHYSICAL_HEIGHT, (float)PHYSICAL_HEIGHT, (float)PHYSICAL_WIDTH };
        DrawTexturePro(target.texture, source, dest, (Vector2){0, 0}, -90.0f, WHITE);
        EndDrawing();
    }

    // Cleanup
    if (midi_ok >= 0) {
        midi_close(&midi);
    }

    UnloadRenderTexture(target);
    StopAudioStream(g_stream);
    UnloadAudioStream(g_stream);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
