#ifndef ARP_H
#define ARP_H

#define ARP_MAX_NOTES 16    // Maximum held notes

typedef enum {
    ARP_UP,
    ARP_DOWN,
    ARP_UP_DOWN,
    ARP_RANDOM,
    ARP_AS_PLAYED,
    ARP_PATTERN_COUNT
} ArpPattern;

typedef enum {
    ARP_DIV_1_4,    // Quarter notes
    ARP_DIV_1_8,    // Eighth notes
    ARP_DIV_1_16,   // Sixteenth notes
    ARP_DIV_1_32,   // Thirty-second notes
    ARP_DIV_COUNT
} ArpDivision;

typedef struct {
    // Settings
    int enabled;
    ArpPattern pattern;
    ArpDivision division;
    float tempo;            // BPM (40-240)
    int octaves;            // 1-4 octaves
    float gate;             // Gate length (0.1-1.0)

    // Note buffer
    int notes[ARP_MAX_NOTES];
    int velocities[ARP_MAX_NOTES];
    int note_count;

    // Playback state
    int current_step;
    int current_octave;
    int direction;          // 1 = up, -1 = down (for up-down pattern)
    float phase;            // 0.0-1.0, advances with tempo
    int last_note;          // Last played note (for note-off)
    int note_on;            // Is a note currently playing?
    unsigned int random_seed;
} Arpeggiator;

// Initialize arpeggiator
void arp_init(Arpeggiator *arp);

// Add/remove notes from buffer
void arp_note_on(Arpeggiator *arp, int note, int velocity);
void arp_note_off(Arpeggiator *arp, int note);
void arp_clear(Arpeggiator *arp);

// Process arpeggiator (call from audio/update loop)
// Returns: 0 = no event, 1 = note on, -1 = note off
// Sets *note and *velocity for note events
int arp_process(Arpeggiator *arp, float delta_time, int *note, int *velocity);

// Get pattern name for UI
const char* arp_pattern_name(ArpPattern pattern);
const char* arp_division_name(ArpDivision div);

#endif // ARP_H
