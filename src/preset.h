#ifndef PRESET_H
#define PRESET_H

#include "synth.h"
#include "effects.h"
#include "arp.h"

#define PRESET_DIR "presets"
#define MAX_PRESETS 99
#define PRESET_NAME_LEN 32

// Save preset to JSON file (returns 0 on success)
int preset_save(const char *filepath, const char *name, Synth *s, Effects *fx, Arpeggiator *arp);

// Load preset from JSON file (returns 0 on success)
int preset_load(const char *filepath, char *name, int name_size, Synth *s, Effects *fx, Arpeggiator *arp);

// Generate preset filename (e.g., "presets/001.json")
void preset_filename(int slot, char *buffer, int buffer_size);

// Check if preset exists
int preset_exists(int slot);

// Get preset name without loading full preset (returns 0 on success)
int preset_get_name(int slot, char *name, int name_size);

#endif // PRESET_H
