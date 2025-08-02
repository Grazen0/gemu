# Gemu

Gemu is a neat little **G**ame Boy **emu**lator written in C as a personal exercise to learn the language.

Name idea blatantly ~~stolen~~ inspired by [QEMU](https://www.qemu.org/).

## Building

If you have [Nix](https://nixos.org/), you already know what to do.

Otherwise, you can clone, compile and run the project with CMake like so:

```bash
git clone https://github.com/Grazen0/gemu.git
cd gemu
cmake . -B build
cmake --build build
build/gemu path/to/rom.gb
```

As per usual, you may install Gemu on your system by selecting the `install` CMake target.

## Credits

- **[Raddad772's sm83 instruction tests](https://github.com/raddad772/jsmoo/tree/b3807b55f03cdad2191810b2a770781d73c41870/misc/tests/GeneratedTests/sm83)**, which are used in this project.
- **[gbdev.io](https://gbdev.io/)** and the **[Pan Docs](https://gbdev.io/pandocs/)** for their incredible help.
