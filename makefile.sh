#!/bin/bash

g++ -c wave_file_reader.c sound_engine.c gpio.c
g++ -lasound -lpthread -o sounder wave_file_reader.o sound_engine.o gpio.o
