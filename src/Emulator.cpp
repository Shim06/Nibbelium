#include <iostream>
#include "Emulator.h"
#include <SDL.h>

int chip8::emulate(std::string game, std::string config_path)
{
    // Load settings from json
    std::ifstream config_file(config_path + "\\config.json");
    if (config_file.is_open())
    {

        // Get values from json file
        nlohmann::json config;
        config_file >> config;
        config_file.close();

        settings.volume = config["volume"];
        settings.display_wait = config["display_wait"];
        IPS = config["IPS"];
        settings.fullscreen = config["start_games_fullscreen"];
        settings.wrapping = config["wrapping"];
        settings.logic = config["logic"];

        pixel_on_R = config["pixel_on_color_R"];
        pixel_on_G = config["pixel_on_color_G"];
        pixel_on_B = config["pixel_on_color_B"];

        pixel_off_R = config["pixel_off_color_R"];
        pixel_off_G = config["pixel_off_color_G"];
        pixel_off_B = config["pixel_off_color_B"];
    }
    else
    {
        return 0;
    }

	// Initialize SDL
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	if (!init_sdl(window, renderer, game, config_path)) return 1;
    if (!init_audio(settings)) return 1;
    if (!init_chip8(game))
    {
        return 1;
    }

    if (settings.fullscreen)
        SDL_SetWindowFullscreen(window,
        SDL_WINDOW_FULLSCREEN_DESKTOP);

    // Determine Instructions per frame
    IPF = IPS / 60;

    running = true;
	// Main emulator loop
	while (running)
    {
		handle_input(window, settings, game);

		if (paused)
			continue;

		const uint64_t start_frame_time = SDL_GetPerformanceCounter();
		// Execute opcodes 
		for (loop_index = 0; loop_index < IPF; loop_index++)
		{
			uint16_t opcode = fetch(PC);
			decode(opcode);
		}
		const uint64_t end_frame_time = SDL_GetPerformanceCounter();

		// Gets the time it took to execute the opcodes
		const double time_elapsed = (double)((end_frame_time - start_frame_time) / 1000) / SDL_GetPerformanceFrequency();

		// Draws the screen
		for (uint8_t y = 0; y < 32; y++)
		{
			for (uint8_t x = 0; x < 64; x++)
			{
				if (display[x][y] == true)
				{
					SDL_SetRenderDrawColor(renderer, pixel_on_R, pixel_on_G, pixel_on_B, 255);
					SDL_RenderDrawPoint(renderer, x, y);
				}
				else
				{
					SDL_SetRenderDrawColor(renderer, pixel_off_R, pixel_off_G, pixel_off_B, 255);
					SDL_RenderDrawPoint(renderer, x, y);
				}
			}
		}

		// Update timers
		if (ST > 0)
		{
			ST--;
			SDL_PauseAudioDevice(dev, 0);
		}
		else
			SDL_PauseAudioDevice(dev, 1);
		if (DT > 0) DT--;

		SDL_RenderPresent(renderer);

		// Delay every 16.67ms to keep the emulator running at 60fps
		uint32_t delay = 16.67f > time_elapsed ? 16.67f - time_elapsed : 0;
		SDL_Delay(delay);
	}

    // Cleanup
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_CloseAudioDevice(dev);
    return 0;
}

bool chip8::init_sdl(SDL_Window*& window, SDL_Renderer*& renderer, std::string game, std::string path)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
	{
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		return false;
	}

    std::filesystem::path file_path = game;
    std::filesystem::path file_name = file_path.stem();
    std::string game_name = file_name.string();

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	window = SDL_CreateWindow(game_name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, window_flags);
	if (!window)
	{
		SDL_Log("Unable to create SDL window: %s", SDL_GetError());
		return false;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer)
	{
		SDL_Log("Unable to create SDL renderer: %s", SDL_GetError());
		return false;
	}

    // Load icon
    SDL_Surface* icon_surface = SDL_LoadBMP((path + "\\icon.bmp").c_str());
    if (icon_surface != NULL) {
        SDL_SetWindowIcon(window, icon_surface);
        SDL_FreeSurface(icon_surface);
    }

	SDL_RenderSetLogicalSize(renderer, 64, 32);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
	return true;
}

bool chip8::init_audio(config config)
{

	// Initialize want
	want.freq = 44100;
	want.format = AUDIO_S16LSB;
	want.channels = 1;
	want.samples = 512;
    want.userdata = this;
	want.callback = chip8::audio_callback;

	// Initialize SDL audio device
	dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	if (dev == 0)
	{
		SDL_Log("Could not create SDL Audio device: %s", SDL_GetError());
		return false;
	}

	if ((want.format != have.format) ||
		(want.channels) != have.channels)
	{
		SDL_Log("Could not get desired audio spec");
		return false;
	}

	return true;
}

bool chip8::init_chip8(std::string game)
{
    // Initialize 4KB memory
    for (uint16_t i = 0; i < 4096; i++)
    {
        memory[i] = 0;
    }

	// Default font for the chip-8
	uint8_t font[80] = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

	// Initialize font
	// Stored at 0x50 to 0x9F
	for (uint8_t i = 0; i < 80; i++)
	{
		memory[i + 0x50] = font[i];
	}

	// Load rom into memory
	std::ifstream ifs;
	ifs.open(game, std::ifstream::binary);
	if (!ifs.is_open())
	{
		return false;
	}

	ifs.seekg(0, std::ios::end);
	std::streampos file_size = ifs.tellg();
	ifs.seekg(0, std::ios::beg);
	ifs.read(reinterpret_cast<char*>(memory + 0x200), file_size);
    ifs.close();

    // Initialize sound timer and delay timer
    ST = 0;
    DT = 0;

    // Initialize registers
    SP = 0;
    I = 0;
    for (uint8_t i = 0; i < 16; i++)
    {
        V[i] = 0;
    }

    // Initialize keypad
    for (uint8_t i = 0; i < 16; i++)
    {
        keypad[i] = false;
    }

    // Initialize display array
    for (uint8_t x = 0; x < 64; x++)
    {
        for (uint8_t y = 0; y < 32; y++)
        {
            display[x][y] = false;
        }
    }

    // Initialize variables
    pressed_key = -1;

    // Initialize stack
    while (!stack.empty())
    {
        stack.pop();
    }

	// Point Program counter to start of memory
	PC = 0x200;
    return true;
}

void chip8::audio_callback(void *userdata, uint8_t *stream, int len)
{
    chip8* instance = static_cast<chip8*>(userdata);
    chip8::config* settings = &(instance->settings);
	int16_t* audio_data = (int16_t*)stream;
	uint32_t running_sample_index = 0;
	int32_t square_wave_period = instance->audio_sample_rate / instance->square_wave_freq;
	int32_t half_square_wave_period = square_wave_period / 2;

	for (int i = 0, length = len / 2; i < length; i++)
	{
		audio_data[i] = ((running_sample_index++ / half_square_wave_period % 2) ? (settings->volume*30) : -(settings->volume*30));
	}
}


void chip8::handle_input(SDL_Window*& window, config& config, std::string game)
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				running = false;
				return;

            case SDL_WINDOWEVENT:
                // Check if it's a window close event
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    running = false;
                    return;
                }
                break;

			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						running = false;
						return;

					// Emulator keypad					
					case SDLK_x: keypad[0x0] = true; break;
					case SDLK_1: keypad[0x1] = true; break;
					case SDLK_2: keypad[0x2] = true; break;
					case SDLK_3: keypad[0x3] = true; break;
					case SDLK_q: keypad[0x4] = true; break;
					case SDLK_w: keypad[0x5] = true; break;
					case SDLK_e: keypad[0x6] = true; break;
					case SDLK_a: keypad[0x7] = true; break;
					case SDLK_s: keypad[0x8] = true; break;
					case SDLK_d: keypad[0x9] = true; break;
					case SDLK_z: keypad[0xA] = true; break;
					case SDLK_c: keypad[0xB] = true; break;
					case SDLK_4: keypad[0xC] = true; break;
					case SDLK_r: keypad[0xD] = true; break;
					case SDLK_f: keypad[0xE] = true; break;
					case SDLK_v: keypad[0xF] = true; break;

					// Pauses or unpauses the game
					case SDLK_F5:
						if (paused) paused = false;
						else paused = true;
						break;

                    // Restarts the loaded ROM
                    case SDLK_t:
                        reset(game);
                        break;
	
					// Goes into fullscreen or windowed mode
					case SDLK_F11:
						if (config.fullscreen)
						{
							config.fullscreen = false;
							SDL_SetWindowFullscreen(window, 0);
						}
						else
						{
							config.fullscreen = true;
							SDL_SetWindowFullscreen(window, 
								SDL_WINDOW_FULLSCREEN_DESKTOP);
						}
						break;

					default: break;
				}
				break;

			case SDL_KEYUP:
				switch (event.key.keysym.sym)
				{
					case SDLK_x: keypad[0x0] = false; break;
					case SDLK_1: keypad[0x1] = false; break;
					case SDLK_2: keypad[0x2] = false; break;
					case SDLK_3: keypad[0x3] = false; break;
					case SDLK_q: keypad[0x4] = false; break;
					case SDLK_w: keypad[0x5] = false; break;
					case SDLK_e: keypad[0x6] = false; break;
					case SDLK_a: keypad[0x7] = false; break;
					case SDLK_s: keypad[0x8] = false; break;
					case SDLK_d: keypad[0x9] = false; break;
					case SDLK_z: keypad[0xA] = false; break;
					case SDLK_c: keypad[0xB] = false; break;
					case SDLK_4: keypad[0xC] = false; break;
					case SDLK_r: keypad[0xD] = false; break;
					case SDLK_f: keypad[0xE] = false; break;
					case SDLK_v: keypad[0xF] = false; break;

					default: break;
				}
				break;
		}
	}
}

void chip8::reset(std::string game)
{
    // Clear registers and screen
    PC = 0x200;
    ST = 0;
    DT = 0;
    SP = 0;
    I = 0;
    clear_screen();

    for (uint8_t i = 0; i < 16; i++)
    {
        V[i] = 0;
    }

    // Clear game memory and stack
    for (uint16_t i = 0x200; i < 4096; i++)
    {
        memory[i] = 0x00;
    }

    while (!stack.empty())
    {
        stack.pop();
    }

    // Reload rom into memory
    std::ifstream ifs;
    ifs.open(game, std::ifstream::binary);
    if (!ifs.is_open())
    {
        return;
    }

    ifs.seekg(0, std::ios::end);
    std::streampos file_size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    ifs.read(reinterpret_cast<char*>(memory + 0x200), file_size);
    ifs.close();
}

uint8_t chip8::read(uint16_t program_counter)
{
	return memory[program_counter];
}

uint16_t chip8::fetch(uint16_t& program_counter)
{
    // Gets the 16 bit instruction
	uint16_t hiByte = read(program_counter);
	program_counter++;
	uint16_t loByte = read(program_counter);
    program_counter++;

	uint16_t opcode = (hiByte << 8) | loByte;
	return opcode;
}

void chip8::decode(uint16_t opcode)
{
    // Decodes and runs the opcode
	switch (opcode & 0xF000)
	{
		case 0x0000:
			switch (opcode & 0x0F00)
			{
				case 0x0000:
					switch (opcode & 0x00FF)
					{
						case 0x00E0:
							clear_screen(); // 00E0
							break;
						case 0x00EE:
							return_from_subroutine(); // 00EE
							break;
					}
					break;
			}
			break;
		case 0x1000:
			jump(opcode & 0x0FFF); // 1NNN
			break;
		case 0x2000:
			call_subroutine(opcode & 0x0FFF); // 2NNN
			break;
		case 0x3000:
			equal_skip((opcode & 0x0F00) >> 8, opcode & 0x00FF); // 3XNN
			break;
		case 0x4000:
			unequal_skip((opcode & 0x0F00) >> 8, opcode & 0x00FF); // 4XNN
			break;
		case 0x5000:
			equal_register_skip((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4); // 5XY0
			break;
		case 0x6000:
			set_vx((opcode & 0x0F00) >> 8, opcode & 0x00FF); // 6XNN
			break;
		case 0x7000:
			add_vx((opcode & 0x0F00) >> 8, opcode & 0x00FF); // 7XNN
			break;
		case 0x8000:
			switch (opcode & 0x000F)
			{
				case 0x0000:
					logical_set((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4); // 8XY0
					break;
				case 0x0001:
					logical_OR((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4); // 8XY1
					break;
				case 0x0002:
					logical_AND((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4); // 8XY2
					break;
				case 0x0003:
					logical_XOR((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4); // 8XY3
					break;
				case 0x0004:
					logical_add((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4); // 8XY4
					break;
				case 0x0005:
					logical_subtract((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4); // 8XY5
					break;
				case 0x0006:
					shift_right((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4); // 8XY6
					break;
				case 0x0007:
					logical_subtract_reverse((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4); // 8XY7
					break;
				case 0x000E:
					shift_left((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4); // 8XYE
					break;
			}
			break;
		case 0x9000:
			unequal_register_skip((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4); // 9XY0
			break;
		case 0xA000:
			set_index(opcode & 0x0FFF); // ANNN
			break;
		case 0xB000:
			offset_jump(opcode & 0x0FFF); // BNNN
			break;
		case 0xC000:
			random((opcode & 0x0F00) >> 8, opcode & 0x00FF); // CXNN
			break;
		case 0xD000:
			draw((opcode & 0x0F00) >> 8, (opcode & 0x00F0) >> 4, opcode & 0x000F); // DXYN
			break;
		case 0xE000:
			switch (opcode & 0x00FF)
			{
				case 0x009E:
					skip_if_key((opcode & 0x0F00) >> 8); // EX9E
					break;
				case 0x00A1:
					skip_if_not_key((opcode & 0x0F00) >> 8); // EXA1
					break;
			}
			break;
		case 0xF000:
			switch (opcode & 0x00FF)
			{
				case 0x0007:
					get_delay_timer((opcode & 0x0F00) >> 8); // FX07
					break;
				case 0x0015:
					set_delay_timer((opcode & 0x0F00) >> 8); // FX15
					break;
				case 0x0018:
					set_sound_timer((opcode & 0x0F00) >> 8); // FX18
					break;
				case 0x0029:
					point_font((opcode & 0x0F00) >> 8); // FX29
					break;
				case 0x0033:
					decimal_conversion((opcode & 0x0F00) >> 8); // FX33
					break;
				case 0x0055:
					store_memory((opcode & 0x0F00) >> 8); // FX55
					break;
				case 0x0065:
					load_memory((opcode & 0x0F00) >> 8); // FX65
					break;
				case 0x001E:
					add_index((opcode & 0x0F00) >> 8); // FX1E
					break;
				case 0x000A:
					get_key((opcode & 0x0F00) >> 8);  // FX0A
					break;
			}
			break;
	}
}

// Opcodes
void chip8::clear_screen()
{
	for (uint8_t x = 0; x < 64; x++)
	{
		for (uint8_t y = 0; y < 32; y++)
		{
		    display[x][y] = false;
		}
	}
}

void chip8::jump(uint16_t address)
{
	PC = address;
}

void chip8::call_subroutine(uint16_t address)
{
	stack.push(PC);
	PC = address;
}

void chip8::return_from_subroutine()
{
	PC = stack.top();
	stack.pop();
}

void chip8::set_vx(uint8_t x, uint8_t value)
{
	V[x] = value;
}

void chip8::add_vx(uint8_t x, uint8_t value)
{
	V[x] += value;
}

void chip8::set_index(uint16_t value)
{
	I = value;
}

void chip8::draw(uint8_t x, uint8_t y, uint8_t n)
{
	if (settings.display_wait == true)
	{
        if (loop_index != 0)
        {
            PC -= 2;
            return;
        }
	}

	uint8_t x_coor = V[x] & 63;
    uint8_t original_x_coor = x_coor;
	uint8_t y_coor = V[y] & 31;
	V[0xF] = 0;

	for (uint8_t i = 0; i < n; i++)
	{
		uint8_t sprite = memory[I + i];
		for (int8_t j = 7; j >= 0; j--)
		{
			uint8_t bit = (sprite >> j) & 0x1;
			if (bit == 1)
			{
                if (!settings.wrapping && x_coor > 63)
                    break;

				if (display[x_coor][y_coor] == 1)
					V[0xF] = 1;
				display[x_coor][y_coor] ^= 1;
			}
			x_coor++;

            if(settings.wrapping)
                x_coor %= 64;
		}
        x_coor = original_x_coor;
		y_coor++;

        if (settings.wrapping)
            y_coor %= 32;
        else if (y_coor > 31)
		{
			break; 
		}
	}
}

void chip8::equal_skip(uint8_t x, uint8_t y)
{
	if (V[x] == y)
	{
		PC += 2;
	}
}

void chip8::unequal_skip(uint8_t x, uint8_t y)
{
	if (V[x] != y)
	{
		PC += 2;
	}
}

void chip8::equal_register_skip(uint8_t x, uint8_t y)
{
	if (V[x] == V[y])
	{
		PC += 2;
	}
}

void chip8::unequal_register_skip(uint8_t x, uint8_t y)
{
	if (V[x] != V[y])
	{
		PC += 2;
	}
}

void chip8::logical_set(uint8_t x, uint8_t y)
{
	V[x] = V[y];
}

void chip8::logical_OR(uint8_t x, uint8_t y)
{
	V[x] |= V[y];
    if (settings.logic == true)
        V[0x0F] = 0;
}

void chip8::logical_AND(uint8_t x, uint8_t y)
{
	V[x] &= V[y];
    if (settings.logic == true)
	    V[0x0F] = 0;
}

void chip8::logical_XOR(uint8_t x, uint8_t y)
{
	V[x] ^= V[y];
    if (settings.logic == true)
	    V[0x0F] = 0;
}

void chip8::logical_add(uint8_t x, uint8_t y)
{
	uint8_t temp = V[x];
	if ((V[x] + V[y]) > 255)
		temp = 1;
	else
		temp = 0;

	V[x] += V[y];
	V[0xF] = temp;
	
}

void chip8::logical_subtract(uint8_t x, uint8_t y)
{
	uint8_t temp;
	if (V[x] >= V[y])
		temp = 1;
	else
		temp = 0;

	V[x] -= V[y];
	V[0xF] = temp;
}

void chip8::logical_subtract_reverse(uint8_t x, uint8_t y)
{
	uint8_t temp;
	if (V[y] >= V[x])
		temp = 1;
	else
		temp = 0;

	V[x] = V[y] - V[x];
	V[0xF] = temp;
}

void chip8::shift_right(uint8_t x, uint8_t y)
{
	V[x] = V[y];
	uint8_t bit = V[x] & 0x01;
	V[x] = V[x] >> 1;
	V[0xF] = bit;
}

void chip8::shift_left(uint8_t x, uint8_t y)
{
	V[x] = V[y];
	uint8_t bit = (V[x] & 0x80) >> 7;
	V[x] = V[x] << 1;
	V[0xF] = bit;
}

void chip8::offset_jump(uint16_t value)
{
	PC = value + V[0];
}

void chip8::random(uint8_t x, uint8_t value)
{
	uint8_t random = (rand() % 0xFF) & value;
	V[x] = random;
}

void chip8::get_delay_timer(uint8_t x)
{
	V[x] = DT;
}

void chip8::set_delay_timer(uint8_t x)
{
	DT = V[x];
}

void chip8::set_sound_timer(uint8_t x)
{
	ST = V[x];
}

void chip8::add_index(uint8_t x)
{
	I += V[x];
	if (I >= 0x1000)
		V[0xF] = 1;
}

void chip8::point_font(uint8_t x)
{
	I = 0x50 + (5 * (V[x] & 0xF));
}

void chip8::decimal_conversion(uint8_t x)
{
	memory[I] = V[x] / 100;
	memory[I + 1] = (V[x] / 10) % 10;
	memory[I + 2] = V[x] % 10;

}

void chip8::store_memory(uint8_t x)
{
	for (uint8_t i = 0; i <= x; i++)
	{
		memory[I] = V[i];
		I++;
	}
}

void chip8::load_memory(uint8_t x)
{
	for (uint8_t i = 0; i <= x; i++)
	{
		V[i] = memory[I];
		I++;
	}
}

void chip8::skip_if_key(uint8_t x)
{
	if (keypad[V[x]] == true)
	{
		PC += 2;
	}
}

void chip8::skip_if_not_key(uint8_t x)
{
	if (keypad[V[x]] == false)
	{
		PC += 2;
	}
}

void chip8::get_key(uint8_t x)
{
	int8_t key_pressed = -1;
    for (uint8_t i = 0; i <= 0x0F; i++)
    {
        if (keypad[i] == true)
        {
            key_pressed = i;
            break;
        }
    }

    if (pressed_key > -1 && keypad[pressed_key] == false)
    {
        V[x] = pressed_key;
        pressed_key = -1;
    }
    else PC -= 2;

    if (key_pressed > -1) pressed_key = key_pressed;
}
