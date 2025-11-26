# ufps

An follow-along implementation of @nathan-baggs [new stream series](https://www.youtube.com/playlist?list=PLmm8GpQIMryAZI9GyroeaLSH5NTxkXHdc)

# Specifics

- In addition to the original, this implementation also has support for Linux. Tested on Arch running Gnome on Wayland.

# Building

This repo can be built with MSVC 14.50 on Windows as well as gcc 15.2.1 on native Linux.

```
make config
make build

```

# Running

As with the original, you will need to supply and build your own resource pack before running.

```
make run
```

After that you can run the game with:

```
make run
```

The resource file needs to be in the same directory as the shader, until they get added to the resource pack.

# Notes

Nathan's implementation, as well as further information can be found at https://github.com/nathan-baggs/ufps.git
