# ghctrl - A Linux Driver for the Wii Guitar Hero Controller #

## 1. Overview ##

A small driver program for Linux and ALSA that enables your guitarhero controller to be used as a MIDI input device.

A user-space driver for GNU/Linux that turns your Wii Guitar Hero Guitar Controller into a bona-fide MIDI instrument. It provides 32 pitches using the regular note buttons, octave-up octave-down using the plus and minus buttons, note-hold and note-hit using the strum up/down lever, and pitchbend.

## 2. Dependencies ##

You need to install the following Debian packages (or equivalent) to compile this driver:

libcwiimote-dev
libasound2-dev

## 3. Usage ##

To compile this driver, just run make. It also comes with a script called connect-wiimote.sh that loads the appropriate Linux Virtual MIDI modules necessary to run this driver. In it, replace the MAC address on the last line with yours and you should be ready to go. If you need help getting this to work, email me.
