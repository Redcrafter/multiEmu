# Multi Emulator
Emulator for multiple consoles written from scratch in C++. \
Currently supporting NES, Gameboy(WIP) and Chip-8

## Planned
Sega Master System \
GBA \
SNES

# Compilation
## Windows
```cmd
git submodule update --init

mkdir build
cd build
cmake ../
```

## Linux
You will need a sound library of your choosing:
```sh
sudo apt install libasound2-dev # alsa
sudo apt install libpulse-dev   # pulse audio
```
As of now OSS and Jack Audio Server have not been tested.

Next you will need to install required display libraries: \
Debian based distros:
```sh
sudo apt install libgtk2.0-dev
```

To clone the repository use the following command:
```sh
git clone --recursive https://github.com/Redcrafter/nesEmu.git
```

If you cloned the repository without the submodules you can use the following command, issued in the repositories root:
```sh
git submodule update --init
```

To compile the project using CMake:
```sh
mkdir build
cd build
cmake ../
make
```

## Mac
Open file dialog currently doesn't work because i don't know how to use cocoa
```sh
git submodule update --init

mkdir build
cd build
cmake ../
make
```
