#include "Instruments.h"

const wchar_t programName[128][24] = {
L"Acoustic Grand Piano",
L"Bright Acoustic Piano",
L"Electric Grand Piano",
L"Honky-tonk Piano",
L"Electric Piano 1",
L"Electric Piano 2",
L"Harpsichord",
L"Clavi",
L"Celesta",
L"Glockenspiel",
L"Music Box",
L"Vibraphone",
L"Marimba",
L"Xylophone",
L"Tubular Bells",
L"Dulcimer",
L"Drawbar Organ",
L"Percussive Organ",
L"Rock Organ",
L"Church Organ",
L"Reed Organ",
L"Accordion",
L"Harmonica",
L"Tango Accordion",
L"Acoustic Guitar (nylon)",
L"Acoustic Guitar (steel)",
L"Electric Guitar (jazz)",
L"Electric Guitar (clean)",
L"Electric Guitar (muted)",
L"Overdriven Guitar",
L"Distortion Guitar",
L"Guitar harmonics",
L"Acoustic Bass",
L"Electric Bass (finger)",
L"Electric Bass (pick)",
L"Fretless Bass",
L"Slap Bass 1",
L"Slap Bass 2",
L"Synth Bass 1",
L"Synth Bass 2",
L"Violin",
L"Viola",
L"Cello",
L"Contrabass",
L"Tremolo Strings",
L"Pizzicato Strings",
L"Orchestral Harp",
L"Timpani",
L"String Ensemble 1",
L"String Ensemble 2",
L"SynthStrings 1",
L"SynthStrings 2",
L"Choir Aahs",
L"Voice Oohs",
L"Synth Voice",
L"Orchestra Hit",
L"Trumpet",
L"Trombone",
L"Tuba",
L"Muted Trumpet",
L"French Horn",
L"Brass Section",
L"SynthBrass 1",
L"SynthBrass 2",
L"Soprano Sax",
L"Alto Sax",
L"Tenor Sax",
L"Baritone Sax",
L"Oboe",
L"English Horn",
L"Bassoon",
L"Clarinet",
L"Piccolo",
L"Flute",
L"Recorder",
L"Pan Flute",
L"Blown Bottle",
L"Shakuhachi",
L"Whistle",
L"Ocarina",
L"Lead 1 (square)",
L"Lead 2 (sawtooth)",
L"Lead 3 (calliope)",
L"Lead 4 (chiff)",
L"Lead 5 (charang)",
L"Lead 6 (voice)",
L"Lead 7 (fifths)",
L"Lead 8 (bass + lead)",
L"Pad 1 (new age)",
L"Pad 2 (warm)",
L"Pad 3 (polysynth)",
L"Pad 4 (choir)",
L"Pad 5 (bowed)",
L"Pad 6 (metallic)",
L"Pad 7 (halo)",
L"Pad 8 (sweep)",
L"FX 1 (rain)",
L"FX 2 (soundtrack)",
L"FX 3 (crystal)",
L"FX 4 (atmosphere)",
L"FX 5 (brightness)",
L"FX 6 (goblins)",
L"FX 7 (echoes)",
L"FX 8 (sci-fi)",
L"Sitar",
L"Banjo",
L"Shamisen",
L"Koto",
L"Kalimba",
L"Bag pipe",
L"Fiddle",
L"Shanai",
L"Tinkle Bell",
L"Agogo",
L"Steel Drums",
L"Woodblock",
L"Taiko Drum",
L"Melodic Tom",
L"Synth Drum",
L"Reverse Cymbal",
L"Guitar Fret Noise",
L"Breath Noise",
L"Seashore",
L"Bird Tweet",
L"Telephone Ring",
L"Helicopter",
L"Applause",
L"Gunshot"
};

const wchar_t drumName[128][17] = {
L"Standard",
L"Standard 2",
L"Standard L/R",
L"",
L"",
L"",
L"",
L"",
L"Room",
L"Hip Hop",
L"Jungle",
L"Techno",
L"Room L/R",
L"House",
L"",
L"",
L"Power",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"Electronic",
L"TR-808",
L"Dance",
L"CR-78",
L"TR-606",
L"TR-707",
L"TR-909",
L"",
L"Jazz",
L"Jazz L/R",
L"",
L"",
L"",
L"",
L"",
L"",
L"Brush",
L"Brush 2",
L"Brush 2 L/R",
L"",
L"",
L"",
L"",
L"",
L"Orchestra",
L"Ethnic",
L"Kick & Snare",
L"Kick & Snare 2",
L"Asia",
L"Cymbal & Claps",
L"Gamelan",
L"Gamelan 2",
L"SFX",
L"Rhythm FX",
L"Rhythm FX 2",
L"Rhythm FX 3",
L"SFX 2",
L"Voice",
L"Cymbal & Claps 2",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"",
L"CM-64/32"
};