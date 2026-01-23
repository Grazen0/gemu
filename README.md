# Gemu

Gemu is a neat little **G**ame Boy **emu**lator written in C as a personal exercise to learn the language.

Name idea blatantly ~~stolen~~ inspired by [QEMU](https://www.qemu.org/).

## Building

If you have [Nix](https://nixos.org/), you already know what to do.

Otherwise, you'll need the following dependencies:

- [CMake](https://cmake.org)
- [SDL3](https://libsdl.org)

If you **don't** explicitly disable testing (via setting `BUILD_TESTING=OFF`), you'll also need these:

- [Unity Test](https://github.com/ThrowTheSwitch/Unity)
- [cJSON](https://github.com/DaveGamble/cJSON)
- [Ruby](https://www.ruby-lang.org) (for running some build scripts)

You can clone, compile and run the project with these commands:

```bash
git clone https://github.com/Grazen0/gemu.git
cd gemu
cmake . -B build
cmake --build build
build/gemu path/to/rom.gb
```

You can install Gemu on your system by choosing the `install` CMake target.

## Progress

> [!NOTE]
> Gemu does not support any mappers as of now, so only games with no mapper (like Tetris) will work.

- [x] CPU emulation
- [x] Custom boot ROM support
- [ ] Graphics
  - [x] Background tiles
  - [x] Objects
  - [ ] Window drawing
  - [ ] Scrolling (still needs support for wrapping)
  - [ ] Proper OAM transfer timing
- [ ] Timers
- [ ] Mappers
  - [ ] MBC1
  - [ ] MBC2
  - [ ] MBC3
  - [ ] MBC4
  - [ ] MBC5
  - [ ] MBC6
  - [ ] MBC7
  - [ ] MMM01
  - [ ] M161
  - [ ] HuC1
  - [ ] HuC-3
  - [ ] Other (?)
- [ ] Interrupts
  - [x] VBlank
  - [ ] STAT
  - [x] Timer
  - [ ] Serial
  - [ ] Joypad
- [ ] Serial transfer
- [ ] Audio
- [ ] CGB support
- [ ] Gamepad support (via SDL)

Some other notes:

- The graphics system right now is very, very fragile. A rewrite would be well-suited.
- Also, a major rework of the timing system should be coming soon&trade;.

## Credits

- **[Raddad772's sm83 instruction tests](https://github.com/raddad772/jsmoo/tree/b3807b55f03cdad2191810b2a770781d73c41870/misc/tests/GeneratedTests/sm83)**, which are used in this project.
- **[gbdev.io](https://gbdev.io/)** and the **[Pan Docs](https://gbdev.io/pandocs/)** for their incredible help.
