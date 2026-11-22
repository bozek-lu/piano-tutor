# Standard MIDI File parser from M5MusicTab5 (MIT, see LICENSE-M5MusicTab5)
idf_component_register(SRCS "engine_midi_smf.c"
                       INCLUDE_DIRS "include"
                       REQUIRES log heap)
