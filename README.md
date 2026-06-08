# Gemu

Gemu is a neat little **G**ame Boy **emu**lator written in C as a personal exercise to learn the language.

Name idea blatantly ~~stolen~~ inspired by [QEMU].

## Building

If you have [Nix], you already know what to do.

Otherwise, you'll need the following dependencies:

- [Meson]
- One of [SDL3], [Raylib], [OpenGL] + [GLUT], [SFML].

If you don't specify `-Dtests=false` when running `meson setup`, you'll also need:

- [Unity Test]
- [cJSON]
- [Ruby] (for running some build scripts)

You can clone, compile and run the project with these commands:

```bash
git clone https://codeberg.org/Grazen0/gemu
cd gemu
meson setup build -Dfrontend=sdl3 # One of sdl3, raylib, opengl, sfml. Defaults to "sdl3".
meson compile -C build
build/gemu path/to/rom.gb
```

Note that you can choose the frontend (`sdl3`, `raylib`, `opengl`, or `sfml`) with `-Dfrontend`.

## Progress

> [!NOTE]
> Gemu does not support any mappers as of now, so only games with no mapper (like Tetris) will work.

- [x] CPU emulation
- [x] Custom boot ROM support
- [ ] Graphics
  - [x] Background tiles
  - [x] Objects
  - [ ] Window drawing
  - [x] Scrolling
  - [ ] Proper OAM transfer timing
- [x] Timers
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

## Credits

- **[Raddad772's sm83 instruction tests](https://github.com/raddad772/jsmoo/tree/b3807b55f03cdad2191810b2a770781d73c41870/misc/tests/GeneratedTests/sm83)**, which are used in this project.
- **[gbdev.io](https://gbdev.io/)** and the **[Pan Docs](https://gbdev.io/pandocs/)** for their incredible help.

[qemu]: https://www.qemu.org/
[nix]: https://nixos.org/
[meson]: https://mesonbuild.com/
[sdl3]: https://libsdl.org/
[raylib]: https://github.com/raysan5/raylib
[opengl]: https://www.opengl.org/
[glut]: https://www.opengl.org/resources/libraries/glut/glut_downloads.php
[sfml]: https://www.sfml-dev.org/
[unity test]: https://github.com/ThrowTheSwitch/Unity
[cjson]: https://github.com/DaveGamble/cJSON
[ruby]: https://www.ruby-lang.org
