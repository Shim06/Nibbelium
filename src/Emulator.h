#ifndef EMULATOR_H
#define EMULATOR_H

#include <chrono>
#include <cmath>
#include <stdint.h>
#include <fstream>
#include "json.hpp"
#include "SDL.h"
#include <stack>
#include <string>
#include <windows.h>

class chip8
{
public:
    struct config
    {
        int volume;
        bool display_wait;
        bool fullscreen;
        bool logic;
        bool wrapping;
    } settings;

    uint8_t memory[4096]; // 4096 bytes or 4 kilobytes of memory
    uint32_t IPS; // Instructions per second
    uint32_t IPF; // Instructions per frame

    // Registers
    uint8_t V[16];
    uint16_t I;
    uint8_t DT; // Delay Timer
    uint8_t ST; // Sound timer
    uint16_t PC; // Program Counter
    uint8_t SP; // Stack Pointer
    std::stack<uint16_t> stack; // Stack
    bool keypad[16]; // Key inputs

    // Display
    bool display[64][32]; // 64x32 display

    // Pixel on color
    uint8_t pixel_on_R;
    uint8_t pixel_on_G;
    uint8_t pixel_on_B;

    // Pixel off color
    uint8_t pixel_off_R;
    uint8_t pixel_off_G;
    uint8_t pixel_off_B;

    uint16_t loop_index;
    bool running = true;
    bool paused = false;
    int8_t pressed_key;

    // Audio sample rate and frequency
    SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;
    int audio_sample_rate = 44100;
    int square_wave_freq = 440;

public:
    int emulate(std::string game, std::string config_path);
    static void audio_callback(void* userdata, uint8_t* stream, int len);

private:
    bool init_sdl(SDL_Window*& window, SDL_Renderer*& renderer, std::string game, std::string path);
    bool init_audio(config config);
    bool init_chip8(std::string game);
    void handle_input(SDL_Window*& window, config& config, std::string game);
    void clear_screen();
    void reset(std::string game);
    uint8_t read(uint16_t program_counter);
    uint16_t fetch(uint16_t& program_counter);
    void decode(uint16_t instruction);

    // Opcodes
    void jump(uint16_t address);
    void call_subroutine(uint16_t address);
    void return_from_subroutine();
    void set_vx(uint8_t x, uint8_t value);
    void add_vx(uint8_t x, uint8_t value);
    void set_index(uint16_t value);
    void draw(uint8_t x, uint8_t y, uint8_t n);
    void equal_skip(uint8_t x, uint8_t y);
    void unequal_skip(uint8_t x, uint8_t y);
    void equal_register_skip(uint8_t x, uint8_t y);
    void unequal_register_skip(uint8_t x, uint8_t y);
    void logical_set(uint8_t x, uint8_t y);
    void logical_OR(uint8_t x, uint8_t y);
    void logical_AND(uint8_t x, uint8_t y);
    void logical_XOR(uint8_t x, uint8_t y);
    void logical_add(uint8_t x, uint8_t y);
    void logical_subtract(uint8_t x, uint8_t y);
    void logical_subtract_reverse(uint8_t x, uint8_t y);
    void shift_right(uint8_t x, uint8_t y);
    void shift_left(uint8_t x, uint8_t y);
    void offset_jump(uint16_t value);
    void random(uint8_t x, uint8_t value);
    void get_delay_timer(uint8_t x);
    void set_delay_timer(uint8_t x);
    void set_sound_timer(uint8_t x);
    void add_index(uint8_t x);
    void point_font(uint8_t x);
    void decimal_conversion(uint8_t x);
    void store_memory(uint8_t x);
    void load_memory(uint8_t x);
    void skip_if_key(uint8_t x);
    void skip_if_not_key(uint8_t x);
    void get_key(uint8_t x);
};

#endif
