#include <vector>

#include "location.h"
#include "rule_sample.h"

class Rule;

class Writer {
    public:
        explicit Writer(bool gbs_mode);

        void record_song_start(const char* out_path);
        void record_song_stop();
        void record_write(unsigned char addr, unsigned char data);
        void record_lcd();
        void write_song_locations();
        void optimize_rule(Rule& rule);
        void optimize_music_stream();
        void write_music_to_disk();

    private:
        FILE* f;

        const bool gbs_mode;

        Location write_location;

        unsigned int regs[0x100];

        std::vector<Location> song_locations;
        std::vector<unsigned int> music_stream;

        int last_music_size;

        SampleRule sample_rule;

        // -----

        void new_bank();
        void write_byte(unsigned int byte);
        void record_byte(unsigned int byte);
        void write_samples();
};
