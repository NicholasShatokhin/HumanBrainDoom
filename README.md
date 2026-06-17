# Human CPU Doom-like Qt PoC

A Qt/C++ proof of concept for the idea: **the human is the CPU, the application is only RAM/VRAM/IO**.

This is not full Doom. It is a tiny Doom-like raycasting toy where the program stores the map, registers, distance buffer and framebuffer, but the human is expected to manually compute the next state and the ray distances.

```text
external memory / map  -> human reads
input/action register  -> human computes NEXT registers
ray micro-instructions -> human computes DIST[0..15]
framebuffer widget     -> program displays what the human wrote
```

The app includes optional `Validate`, `Hint` and `Auto fill` buttons, but these are debugging/learning helpers. The pure mode is manual.

## Build

Requires CMake and Qt Widgets. Qt 6 is preferred; Qt 5 also works.

### Linux / macOS

```bash
mkdir build
cd build
cmake ..
cmake --build . -j
./human_cpu_doom_qt
```

### Windows / Visual Studio generator

```bat
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
Release\human_cpu_doom_qt.exe
```

If CMake cannot find Qt, pass `CMAKE_PREFIX_PATH`, for example:

```bat
cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\6.7.2\msvc2019_64
```

## How to use

1. Choose an action: `w`, `s`, `a`, `d`, or `.`.
2. Manually compute the next player state.
3. Enter `NEXT.X`, `NEXT.Y`, and `NEXT.DIR`.
4. Read the ray micro-instruction table.
5. Manually compute `DIST[0..15]` and type the values into the distance table.
6. The framebuffer updates from your distance buffer.
7. Press `Validate` only if you want to compare against the hidden reference.
8. Press `Commit` to copy `NEXT` into the current player registers.

## Direction encoding

```text
0 = North
1 = East
2 = South
3 = West
```

## Action encoding

```text
w = move forward
s = move backward
a = turn left
d = turn right
. = do nothing
```

## Memory layout

The GUI deliberately exposes memory-like fields:

- current player registers: `P.X`, `P.Y`, `P.DIR`;
- input register: `ACTION`;
- human-written next-state registers: `NEXT.X`, `NEXT.Y`, `NEXT.DIR`;
- human-written display/raycast buffer: `DIST[0..15]`;
- framebuffer display generated only from `DIST[0..15]`.

## Rendering model

To keep it human-computable, this version uses only 16 columns and three ray directions:

```text
columns 0..4    = left diagonal
columns 5..10   = center
columns 11..15  = right diagonal
```

For every column, compute:

```text
DIST[column] = number of grid steps from NEXT position to the first wall
```

The framebuffer turns small distances into taller/darker wall slices and large distances into shorter/lighter ones.

## What counts as human-CPU mode?

Pure mode:

```text
choose ACTION
manually write NEXT
manually write DIST[0..15]
observe framebuffer
Commit
```

Helper/debug mode:

```text
Validate
Hint NEXT
Hint DIST
Auto fill
```

## Roadmap

- editable maps;
- more realistic fixed-point raycasting;
- instruction-by-instruction mode;
- save/load memory snapshots;
- manual stack/register page;
- optional enemies/sprites;
- low-resolution texture memory;
- speedrun mode for “frames manually computed per hour”.
