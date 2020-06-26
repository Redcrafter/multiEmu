# Nes Emulator
A cycle accurate Nes emulator written in C++.
Works for around 92% of games and around 6% of Games have missing mappers.

## Dependencies
- GLEW
- glfw3
- rtaudio
- imgui

# Compilation
## Windows
Good luck getting cmake to find all packages

Install GLEW from http://glew.sourceforge.net/
```cmd
git submodule update --init

mkdir build
cd build
cmake -A (win32|x64) ../
```

## Linux
```sh
#depenging on which audio library you want to use
apt-get install libasound2-dev # alsa (default)
apt-get install libpulse-dev   # pulse audio
# haven't tested OSS / Jack Audio Server

apt-get install libgtk2.0-dev libglew-dev

git submodule update --init

mkdir build
cd build
cmake ../
make
```

## Mac
Open file dialog currently doesn't work because i don't know how to use cocoa
```sh
brew install glew
git submodule update --init

mkdir build
cd build
cmake ../
make
```
