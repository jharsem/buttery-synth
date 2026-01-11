#include "midi.h"
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>

int midi_init(MidiInput *m) {
    snd_seq_t *seq;
    int err;

    m->seq_handle = NULL;
    m->port_id = -1;
    m->connected = 0;

    // Open ALSA sequencer
    err = snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK);
    if (err < 0) {
        fprintf(stderr, "MIDI: Failed to open sequencer: %s\n", snd_strerror(err));
        return -1;
    }

    snd_seq_set_client_name(seq, "ButterySynth");

    // Create input port
    int port = snd_seq_create_simple_port(seq, "MIDI In",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION);

    if (port < 0) {
        fprintf(stderr, "MIDI: Failed to create port: %s\n", snd_strerror(port));
        snd_seq_close(seq);
        return -1;
    }

    m->seq_handle = seq;
    m->port_id = port;

    // Try to auto-connect to first available MIDI input
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;

    // Use malloc instead of alloca (C99 compatibility)
    snd_seq_client_info_malloc(&cinfo);
    snd_seq_port_info_malloc(&pinfo);

    if (!cinfo || !pinfo) {
        fprintf(stderr, "MIDI: Failed to allocate info structures\n");
        if (cinfo) snd_seq_client_info_free(cinfo);
        if (pinfo) snd_seq_port_info_free(pinfo);
        printf("MIDI: No devices found. Waiting for connections...\n");
        printf("MIDI: Connect with: aconnect <source> %d:%d\n",
            snd_seq_client_id(seq), port);
        return 0;
    }

    snd_seq_client_info_set_client(cinfo, -1);
    while (snd_seq_query_next_client(seq, cinfo) >= 0) {
        int client = snd_seq_client_info_get_client(cinfo);

        // Skip our own client
        if (client == snd_seq_client_id(seq)) continue;

        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        while (snd_seq_query_next_port(seq, pinfo) >= 0) {
            unsigned int caps = snd_seq_port_info_get_capability(pinfo);

            // Look for readable MIDI output ports
            if ((caps & SND_SEQ_PORT_CAP_READ) &&
                (caps & SND_SEQ_PORT_CAP_SUBS_READ)) {

                int src_client = snd_seq_port_info_get_client(pinfo);
                int src_port = snd_seq_port_info_get_port(pinfo);

                err = snd_seq_connect_from(seq, port, src_client, src_port);
                if (err >= 0) {
                    printf("MIDI: Connected to %s:%s\n",
                        snd_seq_client_info_get_name(cinfo),
                        snd_seq_port_info_get_name(pinfo));
                    m->connected = 1;
                    snd_seq_client_info_free(cinfo);
                    snd_seq_port_info_free(pinfo);
                    return 0;  // Success, connected
                }
            }
        }
    }

    snd_seq_client_info_free(cinfo);
    snd_seq_port_info_free(pinfo);

    printf("MIDI: No devices found. Waiting for connections...\n");
    printf("MIDI: Connect with: aconnect <source> %d:%d\n",
        snd_seq_client_id(seq), port);

    return 0;  // Success, but not connected yet
}

void midi_close(MidiInput *m) {
    if (m->seq_handle) {
        snd_seq_close((snd_seq_t *)m->seq_handle);
        m->seq_handle = NULL;
    }
}

int midi_poll(MidiInput *m, MidiEvent *event) {
    if (!m->seq_handle) return 0;

    snd_seq_event_t *ev;
    int err = snd_seq_event_input((snd_seq_t *)m->seq_handle, &ev);

    if (err < 0) {
        return 0;  // No event or error
    }

    event->channel = 0;
    event->data1 = 0;
    event->data2 = 0;

    switch (ev->type) {
        case SND_SEQ_EVENT_NOTEON:
            event->type = MIDI_NOTE_ON;
            event->channel = ev->data.note.channel;
            event->data1 = ev->data.note.note;
            event->data2 = ev->data.note.velocity;
            return 1;

        case SND_SEQ_EVENT_NOTEOFF:
            event->type = MIDI_NOTE_OFF;
            event->channel = ev->data.note.channel;
            event->data1 = ev->data.note.note;
            event->data2 = 0;
            return 1;

        case SND_SEQ_EVENT_CONTROLLER:
            event->type = MIDI_CONTROL;
            event->channel = ev->data.control.channel;
            event->data1 = ev->data.control.param;
            event->data2 = ev->data.control.value;
            return 1;

        case SND_SEQ_EVENT_PORT_SUBSCRIBED:
            printf("MIDI: Device connected\n");
            m->connected = 1;
            return 0;

        case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
            printf("MIDI: Device disconnected\n");
            m->connected = 0;
            return 0;

        default:
            return 0;
    }
}
