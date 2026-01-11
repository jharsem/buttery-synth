# ButterySynth Design Document

A 4-voice polyphonic synthesizer for Raspberry Pi with raylib graphics, USB MIDI input, and touch-screen parameter control.

## Specifications

| Feature | Implementation |
|---------|---------------|
| Polyphony | 4 voices |
| Sample Rate | 44100 Hz |
| Audio Buffer | 512 samples (low latency) |
| Display | 1280x400 landscape (DRM) |
| Input | USB MIDI (ALSA), Touch screen |

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                           main.c                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐        │
│  │  raylib  │  │  Audio   │  │   MIDI   │  │  Touch   │        │
│  │  Window  │  │  Stream  │  │  Input   │  │   UI     │        │
│  └──────────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘        │
└─────────────────────┼─────────────┼─────────────┼───────────────┘
                      │             │             │
                      ▼             ▼             ▼
┌─────────────────────────────────────────────────────────────────┐
│                          synth.c                                 │
│                    4-Voice Polyphony Manager                     │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐               │
│  │ Voice 0 │ │ Voice 1 │ │ Voice 2 │ │ Voice 3 │               │
│  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘               │
└───────┼───────────┼───────────┼───────────┼─────────────────────┘
        │           │           │           │
        ▼           ▼           ▼           ▼
┌─────────────────────────────────────────────────────────────────┐
│                          voice.c                                 │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐                │
│  │ Oscillator │─▶│   Filter   │─▶│  Envelope  │─▶ output       │
│  └────────────┘  └────────────┘  └────────────┘                │
└─────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────┐
│                         effects.c                                │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐                │
│  │ Distortion │─▶│   Delay    │─▶│   Reverb   │─▶ audio out    │
│  └────────────┘  └────────────┘  └────────────┘                │
└─────────────────────────────────────────────────────────────────┘
```

## File Descriptions

### Core Audio

| File | Purpose |
|------|---------|
| `oscillator.c` | Phase-accumulator waveform generation (sine, square, saw, triangle, noise) |
| `envelope.c` | ADSR envelope with attack/decay/sustain/release stages |
| `filter.c` | Chamberlin State Variable Filter providing LP/HP/BP simultaneously |
| `voice.c` | Combines oscillator + filter + envelope into a playable voice |
| `synth.c` | Manages 4 voices, handles note allocation and voice stealing |
| `effects.c` | Post-processing: soft-clip distortion, delay line, Schroeder reverb |

### I/O

| File | Purpose |
|------|---------|
| `midi.c` | ALSA sequencer client, auto-connects to USB MIDI devices |
| `ui.c` | Touch-enabled parameter controls and waveform display |
| `main.c` | Raylib initialization, audio callback, main loop |

## DSP Algorithms

### Oscillator (Phase Accumulator)
```
phase += frequency / sample_rate
if phase >= 1.0: phase -= 1.0
output = waveform(phase)
```

### State Variable Filter (Chamberlin)
```
fc = 2 * sin(PI * cutoff_freq / sample_rate)
low  += fc * band
high  = input - low - Q * band
band += fc * high
```

### ADSR Envelope
```
ATTACK:  level += 1.0 / (attack_time * sample_rate)
DECAY:   level -= (1.0 - sustain) / (decay_time * sample_rate)
SUSTAIN: level = sustain_level
RELEASE: level -= level / (release_time * sample_rate)
```

### Schroeder Reverb
```
comb_out = sum of 4 parallel comb filters
output = series of 2 allpass filters on comb_out
```

## UI Layout (1280x400)

```
┌──────────────┬──────────────┬──────────────┬──────────────┐
│  OSCILLATOR  │    FILTER    │   ENVELOPE   │   EFFECTS    │
│              │              │              │              │
│ Wave: [SEL]  │ Type: LP/HP  │ A [=====]    │ Dly T [===]  │
│ Vol  [====]  │ Cut  [=====] │ D [===]      │ Dly M [==]   │
│              │ Reso [===]   │ S [======]   │ Rev   [====] │
│              │              │ R [====]     │ Dist  [=]    │
│              │              │              │ Drive [===]  │
├──────────────┴──────────────┴──────────────┴──────────────┤
│                    WAVEFORM DISPLAY                        │
└────────────────────────────────────────────────────────────┘
```

## MIDI Implementation

### Note Messages
- Note On (0x90): Triggers voice with velocity
- Note Off (0x80): Releases voice

### CC Mappings
| CC | Parameter |
|----|-----------|
| 1  | Filter Cutoff (Mod Wheel) |
| 71 | Filter Resonance |
| 72 | Release Time |
| 73 | Attack Time |
| 74 | Filter Cutoff |
| 91 | Reverb Mix |
| 94 | Delay Mix |

## Build & Run

### Dependencies
```bash
sudo apt install libasound2-dev
```

### Build
```bash
make
```

### Run
```bash
sudo ./buttersynth
```

### Connect MIDI (if not auto-detected)
```bash
# List MIDI devices
aconnect -l

# Connect your device to ButterySynth
aconnect 20:0 128:0  # example: device 20 to ButterySynth
```

## Default Parameters

| Parameter | Default |
|-----------|---------|
| Waveform | Sawtooth |
| Filter Type | Low-pass |
| Filter Cutoff | 0.7 (normalized) |
| Filter Resonance | 0.2 |
| Attack | 10ms |
| Decay | 100ms |
| Sustain | 0.7 |
| Release | 300ms |
| Master Volume | 0.5 |
| Delay Time | 300ms |
| Delay Mix | 0.3 |
| Reverb Mix | 0.2 |
| Distortion Mix | 0.0 (off) |
