# µgame
An follow-along implementation of @nathan-baggs [stream series](https://www.youtube.com/watch?v=TwW9UPc8ImU&list=PLmm8GpQIMryC1Lm_YgphPgzwPue5RAZdi)

# Specifics
- In addition to the original, this implementation also has support for Linux. Tested on Arch running Gnome on Wayland.

# Building
This repo can be built with MSVC on Windows as well as gcc 15.2.1 on native Linux.

```
cd build
cmake ..
cmake --build .


ctest .
```

# Running
As with the original, you will need to supply and build your own resource pack before running.

```
cd build
./tools/resource_packer/resource_packer.exe ../assets/ ./resource
```

After that you can run the game with:

```
cd build
./src/game.exe ../assets/
```
The resource file needs to be in the same directory as the shader, until they get added to the resource pack.

# Notes
Nathan's implementation, as well as further information can be found at https://github.com/nathan-baggs/ugame.git
