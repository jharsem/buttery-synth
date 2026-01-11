#include "arp.h"
#include <stdlib.h>
#include <string.h>

static const char *pattern_names[] = {
    "Up", "Down", "UpDn", "Rand", "Play"
};

static const char *division_names[] = {
    "1/4", "1/8", "1/16", "1/32"
};

// Division multipliers (quarter note = 1.0)
static const float division_mult[] = {
    1.0f,   // 1/4
    2.0f,   // 1/8
    4.0f,   // 1/16
    8.0f    // 1/32
};

void arp_init(Arpeggiator *arp) {
    arp->enabled = 0;
    arp->pattern = ARP_UP;
    arp->division = ARP_DIV_1_8;
    arp->tempo = 120.0f;
    arp->octaves = 1;
    arp->gate = 0.5f;

    arp->note_count = 0;
    arp->current_step = 0;
    arp->current_octave = 0;
    arp->direction = 1;
    arp->phase = 0.0f;
    arp->last_note = -1;
    arp->note_on = 0;
    arp->random_seed = 12345;
}

// Sort notes in buffer (for Up/Down patterns)
static void sort_notes(Arpeggiator *arp) {
    // Simple bubble sort (small array)
    for (int i = 0; i < arp->note_count - 1; i++) {
        for (int j = 0; j < arp->note_count - i - 1; j++) {
            if (arp->notes[j] > arp->notes[j + 1]) {
                int tmp_note = arp->notes[j];
                int tmp_vel = arp->velocities[j];
                arp->notes[j] = arp->notes[j + 1];
                arp->velocities[j] = arp->velocities[j + 1];
                arp->notes[j + 1] = tmp_note;
                arp->velocities[j + 1] = tmp_vel;
            }
        }
    }
}

void arp_note_on(Arpeggiator *arp, int note, int velocity) {
    // Check if note already in buffer
    for (int i = 0; i < arp->note_count; i++) {
        if (arp->notes[i] == note) {
            arp->velocities[i] = velocity;
            return;
        }
    }

    // Add new note if buffer not full
    if (arp->note_count < ARP_MAX_NOTES) {
        arp->notes[arp->note_count] = note;
        arp->velocities[arp->note_count] = velocity;
        arp->note_count++;

        // Sort for Up/Down patterns (keep As-Played order intact by sorting copy)
        if (arp->pattern != ARP_AS_PLAYED) {
            sort_notes(arp);
        }
    }
}

void arp_note_off(Arpeggiator *arp, int note) {
    // Find and remove note
    for (int i = 0; i < arp->note_count; i++) {
        if (arp->notes[i] == note) {
            // Shift remaining notes down
            for (int j = i; j < arp->note_count - 1; j++) {
                arp->notes[j] = arp->notes[j + 1];
                arp->velocities[j] = arp->velocities[j + 1];
            }
            arp->note_count--;

            // Reset step if it's now out of bounds
            if (arp->current_step >= arp->note_count && arp->note_count > 0) {
                arp->current_step = 0;
                arp->current_octave = 0;
            }
            return;
        }
    }
}

void arp_clear(Arpeggiator *arp) {
    arp->note_count = 0;
    arp->current_step = 0;
    arp->current_octave = 0;
    arp->direction = 1;
    arp->phase = 0.0f;
    arp->last_note = -1;
    arp->note_on = 0;
}

// Simple random number generator
static int arp_random(Arpeggiator *arp, int max) {
    arp->random_seed ^= arp->random_seed << 13;
    arp->random_seed ^= arp->random_seed >> 17;
    arp->random_seed ^= arp->random_seed << 5;
    return arp->random_seed % max;
}

// Get next step based on pattern
static void advance_step(Arpeggiator *arp) {
    int total_steps = arp->note_count * arp->octaves;
    if (total_steps == 0) return;

    switch (arp->pattern) {
        case ARP_UP:
        case ARP_AS_PLAYED:
            arp->current_step++;
            if (arp->current_step >= arp->note_count) {
                arp->current_step = 0;
                arp->current_octave++;
                if (arp->current_octave >= arp->octaves) {
                    arp->current_octave = 0;
                }
            }
            break;

        case ARP_DOWN:
            arp->current_step--;
            if (arp->current_step < 0) {
                arp->current_step = arp->note_count - 1;
                arp->current_octave--;
                if (arp->current_octave < 0) {
                    arp->current_octave = arp->octaves - 1;
                }
            }
            break;

        case ARP_UP_DOWN:
            arp->current_step += arp->direction;
            if (arp->direction > 0) {
                // Going up
                if (arp->current_step >= arp->note_count) {
                    arp->current_octave++;
                    if (arp->current_octave >= arp->octaves) {
                        // Reverse direction
                        arp->current_octave = arp->octaves - 1;
                        arp->current_step = arp->note_count - 2;
                        if (arp->current_step < 0) arp->current_step = 0;
                        arp->direction = -1;
                    } else {
                        arp->current_step = 0;
                    }
                }
            } else {
                // Going down
                if (arp->current_step < 0) {
                    arp->current_octave--;
                    if (arp->current_octave < 0) {
                        // Reverse direction
                        arp->current_octave = 0;
                        arp->current_step = 1;
                        if (arp->current_step >= arp->note_count) arp->current_step = 0;
                        arp->direction = 1;
                    } else {
                        arp->current_step = arp->note_count - 1;
                    }
                }
            }
            break;

        case ARP_RANDOM:
            arp->current_step = arp_random(arp, arp->note_count);
            arp->current_octave = arp_random(arp, arp->octaves);
            break;

        default:
            break;
    }
}

int arp_process(Arpeggiator *arp, float delta_time, int *note, int *velocity) {
    if (!arp->enabled || arp->note_count == 0) {
        // If there was a note playing, turn it off
        if (arp->note_on && arp->last_note >= 0) {
            *note = arp->last_note;
            *velocity = 0;
            arp->note_on = 0;
            arp->last_note = -1;
            return -1;  // Note off
        }
        return 0;  // No event
    }

    // Calculate step duration in seconds
    // BPM = beats per minute, division_mult converts to note division
    float beats_per_sec = arp->tempo / 60.0f;
    float steps_per_sec = beats_per_sec * division_mult[arp->division];
    float step_duration = 1.0f / steps_per_sec;
    // gate_duration not needed - we compare phase against gate directly

    // Advance phase
    float old_phase = arp->phase;
    arp->phase += delta_time / step_duration;

    // Check for note off (gate ended)
    if (arp->note_on && old_phase < arp->gate && arp->phase >= arp->gate) {
        *note = arp->last_note;
        *velocity = 0;
        arp->note_on = 0;
        return -1;  // Note off
    }

    // Check for new step (phase wrapped)
    if (arp->phase >= 1.0f) {
        arp->phase -= 1.0f;

        // Advance to next step
        advance_step(arp);

        // Clamp step to valid range
        if (arp->current_step < 0) arp->current_step = 0;
        if (arp->current_step >= arp->note_count) arp->current_step = arp->note_count - 1;

        // Play new note
        int base_note = arp->notes[arp->current_step];
        int octave_offset = arp->current_octave * 12;
        *note = base_note + octave_offset;
        *velocity = arp->velocities[arp->current_step];

        arp->last_note = *note;
        arp->note_on = 1;

        return 1;  // Note on
    }

    return 0;  // No event
}

const char* arp_pattern_name(ArpPattern pattern) {
    if (pattern >= ARP_PATTERN_COUNT) return "???";
    return pattern_names[pattern];
}

const char* arp_division_name(ArpDivision div) {
    if (div >= ARP_DIV_COUNT) return "???";
    return division_names[div];
}
