# Multi Emulator
Emulator for multiple consoles written from scratch in C++. \
Currently supporting NES and Chip-8.

The NES emulator works for around 92% of games and around 6% of Games have missing mappers.

# Compilation
## Windows
```cmd
git submodule update --init

mkdir build
cd build
cmake -A (win32|x64) ../
```

## Linux
```sh
# depenging on which audio libraries you want to use
apt-get install libasound2-dev # alsa (default)
apt-get install libpulse-dev   # pulse audio
# haven't tested OSS / Jack Audio Server

apt-get install libgtk2.0-dev

git submodule update --init

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
