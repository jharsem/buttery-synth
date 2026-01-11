#ifndef MIDI_H
#define MIDI_H

// MIDI message types
#define MIDI_NOTE_OFF     0x80
#define MIDI_NOTE_ON      0x90
#define MIDI_CONTROL      0xB0

// Common CC numbers
#define CC_MOD_WHEEL      1
#define CC_FILTER_CUTOFF  74
#define CC_FILTER_RESO    71
#define CC_ATTACK         73
#define CC_RELEASE        72
#define CC_REVERB         91
#define CC_DELAY          94

typedef struct {
    void *seq_handle;  // snd_seq_t*
    int port_id;
    int connected;
} MidiInput;

typedef struct {
    int type;       // MIDI_NOTE_ON, MIDI_NOTE_OFF, MIDI_CONTROL
    int channel;    // 0-15
    int data1;      // note or CC number
    int data2;      // velocity or CC value
} MidiEvent;

int midi_init(MidiInput *m);
void midi_close(MidiInput *m);

// Returns 1 if event received, 0 if none available
int midi_poll(MidiInput *m, MidiEvent *event);

#endif // MIDI_H
