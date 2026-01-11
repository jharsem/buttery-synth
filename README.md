# ButterySynth

A 4-voice polyphonic synthesizer for Raspberry Pi with touchscreen control, designed for the WaveShare 400x1280 display.

## Features

### Sound Engine
- **Dual Oscillators** - Sine, square, saw, triangle, noise, and wavetable waveforms
- **Wavetable Synthesis** - 4 built-in tables (Basic, PWM, Harmonics, Formant) with position morphing
- **Pulse Width Modulation** - Variable pulse width for square waves with LFO modulation
- **Unison/Super-Saw** - Up to 7 stacked oscillators with spread detuning
- **Oscillator Mix** - Blend between OSC1 and OSC2
- **Detune** - ±100 cents for rich unison sounds
- **Sub-Oscillator** - Octave-down for bass weight
- **State Variable Filter** - Lowpass, highpass, bandpass modes with resonance
- **Filter Envelope** - Dedicated ADSR with bipolar modulation depth
- **LFO** - Sine, triangle, saw, square waveforms for filter modulation
- **Amplitude Envelope** - Full ADSR control

### Arpeggiator
- **Patterns** - Up, Down, Up-Down, Random, As-Played
- **Divisions** - 1/4, 1/8, 1/16, 1/32 notes
- **Tempo** - 40-240 BPM
- **Octave Range** - 1-4 octaves
- **Gate Length** - Adjustable note duration

### Effects
- **Delay** - Time and feedback control
- **Reverb** - Schroeder-style with room size
- **Distortion** - Soft-clip waveshaping with drive control

### Preset System
- 99 preset slots with JSON storage
- On-screen keyboard for naming presets
- 10 factory presets included

### Settings
- Adjustable audio buffer (512/256/128 samples)
- PANIC button for all-notes-off

## Requirements

### Hardware
- Raspberry Pi (tested on Pi 4)
- WaveShare 400x1280 touchscreen display
- USB MIDI controller

### Software
- Raspberry Pi OS
- Raylib (DRM platform build)
- ALSA development libraries

## Building

### Install Dependencies
```bash
sudo apt-get install libasound2-dev libdrm-dev libgbm-dev libegl-dev libgles2-mesa-dev
```

### Build Raylib for DRM
```bash
git clone https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_DRM
```

### Build ButterySynth
```bash
cd ButterySynth
make
```

## Running

```bash
sudo ./buttersynth
```

Root access is required for DRM display and raw input access.

### Connect MIDI
After starting, connect your MIDI controller:
```bash
aconnect -l                    # List MIDI ports
aconnect <your-device>:0 128:0 # Connect to synth
```

## Controls

### UI Pages

| Tab | Description |
|-----|-------------|
| OSC | Oscillator waveforms, wavetable, mix, detune, unison, sub-oscillator |
| FLT | Filter type, cutoff, resonance, amplitude envelope |
| FX  | Delay, reverb, distortion |
| MOD | LFO rate/depth, filter envelope, PWM controls |
| ARP | Arpeggiator on/off, pattern, tempo, octaves, gate |
| PRE | Preset load/save with name editing |
| SET | Buffer size, panic button |

### MIDI CC Mapping

| CC | Function |
|----|----------|
| 1  | Filter cutoff (mod wheel) |
| 74 | Filter cutoff |
| 71 | Filter resonance |
| 73 | Attack |
| 72 | Release |
| 91 | Reverb mix |
| 92 | Delay mix |

## Factory Presets

| Slot | Name | Description |
|------|------|-------------|
| 001 | INIT | Clean starting point |
| 002 | FAT BASS | Thick bass with sub + distortion |
| 003 | SOFT PAD | Slow attack, lush reverb |
| 004 | BRIGHT LEAD | Resonant saw with vibrato |
| 005 | PLUCK | Short decay for arpeggios |
| 006 | WOBBLE BASS | LFO-modulated dubstep bass |
| 007 | STRINGS | Detuned saws with reverb |
| 008 | ACID | Classic 303-style squelch |
| 009 | DARK DRONE | Deep, slow-evolving ambient |
| 010 | SYNC LEAD | Heavily detuned, aggressive |

## Project Structure

```
ButterySynth/
├── src/
│   ├── main.c          # Entry point, audio/MIDI/display setup
│   ├── synth.c/h       # Voice management, global parameters
│   ├── voice.c/h       # Individual voice processing
│   ├── oscillator.c/h  # Waveform generation
│   ├── wavetable.c/h   # Wavetable synthesis
│   ├── envelope.c/h    # ADSR envelope
│   ├── filter.c/h      # State variable filter
│   ├── lfo.c/h         # Low frequency oscillator
│   ├── arp.c/h         # Arpeggiator
│   ├── effects.c/h     # Delay, reverb, distortion
│   ├── preset.c/h      # JSON preset save/load
│   ├── midi.c/h        # ALSA MIDI input
│   └── ui.c/h          # Touchscreen UI
├── presets/            # JSON preset files
├── Makefile
└── README.md
```

## Performance

Optimized for low-latency on Raspberry Pi:
- Filter coefficients cached (no per-sample trig)
- Tanh lookup table for distortion
- Configurable buffer size down to 128 samples (~2.9ms latency)

## License

MIT License
