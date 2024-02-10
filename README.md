# Nibellium
A CHIP-8 Emulator written in C++ with Dear ImGui and SDL2 for Windows.

https://github.com/Shim06/Nibbelium/assets/50046854/15347727-592b-4a27-baa3-056b1f6e18d5

Nibellium is an Emulator for the old CHIP-8 virtual machine, that was initially used on the COSMAC VIP. This emulator was written in C++ to optimize for emulator performance, while also using Dear ImGui and SDL2 for its GUI and graphics. Nibellium contains a simplistic GUI to make the emulator easy to use and convenient to use. Various games are also included with the emulator to allow users to try out different games. It has different settings to change certain behaviours of the emulator such as, the pixel colors, the amount of instructions run per second, and the different CHIP-8 quirks. This software aims to emulate the old CHIP-8 virtual machine as accurately as possible and to achieve a high game compatability rate.

## Introduction
CHIP-8 was most commonly implemented on 4K systems, such as the Cosmac VIP and the Telmac 1800. These machines had 4096 (0x1000) memory locations, all of which are 8 bits (a byte) which is where the term CHIP-8 originated.

In the emulation development community, the CHIP-8 is considered to be the "Hello, world" of the emulation world. It teaches many low-level and fundemental computer concepts such as CPU registers, memory, opcode decoding, and instruction execution. Furthermore, you learn about bitwise operations and how bits and addresses are used by the CPU at a low level, bringing more perspective when coding. This allows you to think more deeply about how to optimize and code. By creating a CHIP-8 Emulator it will give you the foundation needed to create emulators of more complex systems such as the NES. This is why I chose to create and develop a CHIP-8 Emulator.

## Included games
The CHIP-8 has various game roms that are in the public domain. As such, this emulator has various games pre-included with the release.

## Accuracy
To ensure accurate emulation, multiple test roms that were created by the emulation community were used. These test roms would test if all the opcodes were working properly, if registers were being set properly, if the sound timer and delay timer were decremented properly, and if the quirks of the CHIP-8 were correctly emulated.

![Opcode Test](/assets/Corax+%20opcode%20test.png)

![Flags Test](/assets/Flags%20test.png)

![Quirks Test](/assets/Flags%20test.png)

![Keypad Test](/assets/Keypad%20test.png)

![GetKey Test](/assets/GetKey%20test.png)

More information can be read at https://github.com/Timendus/chip8-test-suite

## Controls

The earliest computers that CHIP-8 were used with had hexadecimal keypads. These had 16 keys, labelled `0` through `F`, and were arranged in a 4x4 grid.

![Keyboard](/assets/cosmac-vip-keypad.png)

 The emulator uses the left side of the keyboard correspondingly for its controls.

| 1 | 2 | 3 | 4 |
|:-:|:-:|:-:|:-:|
| Q | W | E | R |
| A | S | D | F |
| Z | X | C | V |


## Instructions/How to use
Once the application is opened, you will be greeted to a list of games and a menu bar. The menu bar contains File and Options.

### File
- File contains 2 functions. Loading Chip-8 Roms, and opening the game directory. Choosing load Chip-8 rom will prompt the user to select a file with the .ch8 extension using the windows file explorer.
- Opening the game directory will open the directory where the emulator checks for CHIP-8 games. Adding games to this game directory will also add them to the list in the main menu.

### Options
- Options contains 3 functions. Entering fullscreen with the application, selecting to start loaded games in fullscreen, and opening the settings menu.
- The settings menu contains various settings that will affect the behaviour of the emulator.

### Settings
`Pixel on color`
        - This is the color the emulator displays when a pixel is turned on

`Pixel off color`
        - This is the color the emulator displays when a pixel is turned off

`Volume`
        - This controls the volume of the emulator

`Instructions per second`
        - This is the rate at which the emulator executes instructions. Different games may require differing amounts of IPS to work at the correct speed.

`Display Wait`
        - This is used for old CHIP-8 games to emulate more accurately the original CHIP-8 Interpreter of the COSMAC VIP. It would use this to avoid screen tearing in games. However, most modern emulators usually don't require this turned on.

`Logic Quirk`
        - This is a quirk switch that stops the VF register from being reset if set to false. This is usually used for modern CHIP-8 games.

`Wrapping Quirk`
        - This would make sprites wrap around to the other side of the screen. The original CHIP-8 Interpreter did not wrap sprites.


## Contributing

Contributions are welcome. For major changes, please open an issue first to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details
