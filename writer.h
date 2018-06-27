#pragma once

void record_song_start(const char* out_path);
void record_song_stop();
void record_write(char addr, char data);
void record_lcd();
void record_complete();
