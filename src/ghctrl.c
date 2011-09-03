/* vim:set cindent:set sw=4:set sts=4 */
/******************************************************************************
 * Copyright (c) 2008, 2009 Christopher C. Eineke <chris@chriseineke.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

/*
 * This file implements a userspace driver for the Wii's Guitar Hero
 * Controller. It requires the following extra Debian packages to compile:
 *   - libcwiimote-dev
 *   - libasound2-dev
 */

#include <alsa/asoundlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiimote_api.h>
#include "print.h"

/* The MAC address of the wiimote that is attached to the GH controller. */
static char* wiimote_mac_addr = "00:19:1D:8C:9A:87";

/* The ALSA device we will transmit MIDI data on. */
static char* alsa_hw_dev;

static void exit_with_usage(int result);
static void parse_command_line_arguments(int ac, char* av[]);

static void init_wiimote(wiimote_t** wm)
{
	printf("Waiting for wiimote %s... (press 1 + 2)\n", wiimote_mac_addr);
	*wm = wiimote_open(wiimote_mac_addr);
	if (!*wm) {
		exit(1);
	}
	printf("Connected to wiimote %s.\n", wiimote_mac_addr);
}

static void init_rawmidi(snd_rawmidi_t** hndl)
{
	int ret = snd_rawmidi_open(NULL, hndl, alsa_hw_dev, 0);

	if (ret) {
		printf("snd_rawmidi_open %s failed: %s\n", alsa_hw_dev, snd_strerror(ret));
		exit(1);
        }
}

typedef struct {
	union {
		struct {
			bool green      : 1;
			bool red        : 1;
			bool yellow     : 1;
			bool blue       : 1;
			bool orange     : 1;
			bool strum_up   : 1;
			bool strum_down : 1;
		};
		uint8_t bits;
	};
	bool minus;
	bool plus;
} guitar_buttons_t;

typedef struct {
	guitar_buttons_t button;
	uint8_t whammie;
	int8_t pitch_offset;
} guitar_t;

void wiimote_to_ghc(const wiimote_t* wm, guitar_t* gui)
{
	uint8_t bits = wm->ext.nunchuk.keys.bits;
	
	gui->button.green      = (bits >> 4) & 0x01;
	gui->button.red        = (bits >> 6) & 0x01;
	gui->button.yellow     = (bits >> 3) & 0x01;
	gui->button.blue       = (bits >> 5) & 0x01;
	gui->button.orange     = (bits >> 7) & 0x01;
	gui->button.strum_up   = wm->ext.nunchuk.keys.z;
	gui->button.strum_down = wm->ext.nunchuk.axis.z == 191 ? 1 : 0;
	gui->button.minus      = ~wm->ext.nunchuk.axis.z >> 4 & 0x01;
	gui->button.plus       = ~wm->ext.nunchuk.axis.z >> 2 & 0x01;
	gui->whammie           = wm->ext.nunchuk.axis.y - 240;
}

void update_ghc_midi_state(guitar_t* cur, const guitar_t* const prev)
{
	if (cur->button.plus > prev->button.plus) {
		cur->pitch_offset++;
	}
	if (cur->button.minus > prev->button.minus) {
		cur->pitch_offset--;
	}
}

void dump_controller_state(const guitar_t cur)
{
	printf("green:%d red:%d yellow:%d blue:%d orange: %d"
			" strum_up:%d strum_down:%d bits:%d "
			" minus:%d plus:%d whammie:%d offset:%d\n",
		cur.button.green,
		cur.button.red,
		cur.button.yellow,
		cur.button.blue,
		cur.button.orange,
		cur.button.strum_up,
		cur.button.strum_down,
		cur.button.bits,
		cur.button.minus,
		cur.button.plus,
		cur.whammie,
		cur.pitch_offset);
}

unsigned char last_pitch = 0x3c;

void emit_note_on(snd_rawmidi_t* midi_out, const guitar_t cur, const guitar_t prev)
{
	unsigned char channel = 0x90; /* 9 = note on, 0 = channel 1 */
	unsigned char pitch = 0x3c; /* 0x3c = middle c */
	unsigned char velocity = 0x40; /* 0x40 = velocity 64 */

	pitch += cur.button.bits & 0x1F;
	pitch += cur.pitch_offset * 12;

	unsigned char buf[3] = { channel, pitch, velocity };
	ssize_t wrote = snd_rawmidi_write(midi_out, &buf, 3);
	
	printf("NOTE-ON { %x %x %x } (%ld bytes).\n", buf[0], buf[1], buf[2], wrote);
	
	last_pitch = pitch;
}

void emit_note_off(snd_rawmidi_t* midi_out, const guitar_t cur, const guitar_t prev)
{
	unsigned char channel = 0x90; /* 9 = note on, 0 = channel 1 */
//	unsigned char pitch = 0x3c; /* 0x3c = middle c */
	unsigned char pitch = last_pitch;
	unsigned char velocity = 0x00; /* 0x00 = note off */

//	pitch += cur.button.bits & 0x1F;

	unsigned char buf[3] = { channel, pitch, velocity };
	ssize_t wrote = snd_rawmidi_write(midi_out, &buf, 3);
	
	printf("NOTE-OFF { %x %x %x } (%ld bytes).\n", buf[0], buf[1], buf[2], wrote);
}

void emit_pitch_bend(snd_rawmidi_t* midi_out, const guitar_t cur, const guitar_t prev)
{
	unsigned char channel = 0xE0; /* E = pitchbend, 0 = channel 1 */
	union {
		uint16_t bend;
		char ch[2];
	} pb = {
		.bend = 0x4000, /* no pitch bend */
	};

	pb.bend += cur.whammie * 1024;

	unsigned char buf[3] = { channel, pb.ch[0], pb.ch[1] };
	ssize_t wrote = snd_rawmidi_write(midi_out, &buf, 3);
	
	printf("PITCHBEND { %x %x %x } (%ld bytes).\n", buf[0], buf[1], buf[2], wrote);
}

void send_midi_events(snd_rawmidi_t* midi_out, guitar_t cur, const guitar_t prev)
{	
	/**
	 * Strum down -> note on
	 * Strum back to center, after down -> note off
	 *
	 * Strum up -> note on, followed by note off
	 * Strum back to center, after up -> nothing
	 */
	if (cur.button.strum_down > prev.button.strum_down) {
		emit_note_on(midi_out, cur, prev);
	}
	else if (cur.button.strum_down < prev.button.strum_down) {
		emit_note_off(midi_out, cur, prev);
	}
	else if (cur.button.strum_down == prev.button.strum_down) {
		/* ignore */
	}
	
	if (cur.button.strum_up > prev.button.strum_up) {
		emit_note_on(midi_out, cur, prev);
		emit_note_off(midi_out, cur, prev);
	}
	else if ((cur.button.strum_up == prev.button.strum_up)
			|| (cur.button.strum_up < prev.button.strum_up)) {
		/* ignore */
	}

	if (cur.whammie != prev.whammie) {
		emit_pitch_bend(midi_out, cur, prev);
	}
}

void send_wiimote_events(wiimote_t* wm, guitar_t cur)
{
	wm->led.one    = cur.button.green;
	wm->led.two    = cur.button.red;
	wm->led.three  = cur.button.yellow;
	wm->led.four   = cur.button.blue;
	wm->led.rumble = cur.button.orange;
}

int main(int ac, char* av[])
{
	snd_rawmidi_t* handle_out = NULL;
	wiimote_t* wm = NULL;
	guitar_t cur;
	guitar_t prev;

	parse_command_line_arguments(ac, av);
	init_wiimote(&wm);
	init_rawmidi(&handle_out);
	memset(&prev, 0x00, sizeof(prev));
	memset(&cur, 0x00, sizeof(cur));

	while (wiimote_is_open(wm)) {
		wiimote_update(wm);
		wiimote_to_ghc(wm, &cur);
		update_ghc_midi_state(&cur, &prev);

		//dump_controller_state(cur);

		send_midi_events(handle_out, cur, prev);
		send_wiimote_events(wm, cur);

		prev = cur;
	}

	wiimote_close(wm);

	return 0;
}

static void exit_with_usage(int result)
{	/**
	 * Print a short message that explains the program options and exit
	 * back to the operating system.
	 */
	printf("usage:\n");
	printf("\t--help (print this help text)\n");
	printf("\t--wiimote <MAC> (the Wiimote's MAC address)\n");
	printf("\t--device <name> (the ALSA device)\n");
	exit(result);
}

static void parse_command_line_arguments(int ac, char* av[])
{
	/*
	 * Option enum:
	 *   Symbols that are passed to getopt to identify what option is being parsed
	 *   at the moment.
	 */
	enum {
		OPT_NONE,
		OPT_HELP,
		OPT_WIIMOTE,
		OPT_DEVICE,
		OPT_ALL,
	};

	struct option long_opts[] = {
		{ "help",    1, NULL, OPT_HELP },
		{ "wiimote", 1, NULL, OPT_WIIMOTE },
		{ "device",  1, NULL, OPT_DEVICE },
		{ NULL, 0, NULL, 0 },
	};

	int getopt_ret = 0;
	int getopt_longindex = 0;

	if (ac < 2) {
		exit_with_usage(EXIT_FAILURE);
	}

	while ((getopt_ret = getopt_long_only(ac, av, "", long_opts, &getopt_longindex)) != -1) {
		switch (getopt_ret) {
		case '?':
			exit_with_usage(EXIT_FAILURE);
			break;
		case OPT_HELP:
			exit_with_usage(EXIT_SUCCESS);
			break;
		case OPT_WIIMOTE:
			wiimote_mac_addr = strdup(optarg);
			print_debug("Set wiimote mac address to %s\n", wiimote_mac_addr);
			break;
		case OPT_DEVICE:
			alsa_hw_dev = strdup(optarg);
			print_debug("Set ALSA device %s\n", alsa_hw_dev);
			break;
		default:
			exit_with_usage(EXIT_FAILURE);
			break;
		}
	}
}
