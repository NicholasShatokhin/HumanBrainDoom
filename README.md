# Human CPU Doom-like PoC

A tiny C++ proof of concept for the idea: **the human is the CPU, the program is only external memory, display, and optional validation**.

This is not full Doom. It is a deliberately small Doom-like/raycasting toy where a person manually computes the next state and the wall distances for a low-resolution ASCII frame.

```text
map + registers + framebuffer live in the program
human reads them
human computes the next state
human writes the results back
program renders the frame from those manually entered values
```

The goal is to make the phrase “Doom running on a human brain” technically meaningful in a minimal, playable, inspectable form.

## Concept

In a normal game loop, the computer does this:

```text
input -> update game state -> raycast/render -> display frame
```

In this project, the loop is split:

```text
input -> human computes update -> human computes ray distances -> program displays frame
```

The program stores:

- the map;
- player registers;
- the next-state registers;
- a 16-column distance buffer;
- an ASCII framebuffer.

The human manually fills:

- the next player position and direction;
- the distance from the player to the first wall for each screen column.

The program can optionally validate your calculation against a hidden built-in reference implementation, but validation is not required for the “human CPU” mode.

## Screenshot

Example ASCII frame:

```text
+----------------+
|                |
|                |
|     ######     |
|.....######.....|
|.....######.....|
|.....######.....|
|.....######.....|
|_____######_____|
|________________|
|________________|
|________________|
|________________|
+----------------+
```

## Build

Requires a C++17 compiler.

### Linux / macOS

```bash
g++ -std=c++17 -O2 human_cpu_doom_poc.cpp -o human_doom
./human_doom
```

### Windows / MinGW

```bat
g++ -std=c++17 -O2 human_cpu_doom_poc.cpp -o human_doom.exe
human_doom.exe
```

### Windows / MSVC Developer Command Prompt

```bat
cl /std:c++17 /O2 human_cpu_doom_poc.cpp /Fe:human_doom.exe
human_doom.exe
```

## How to play manually

When the program starts, it prints the map, registers, and current frame.

The player direction is encoded as:

```text
0 = North
1 = East
2 = South
3 = West
```

Actions are:

```text
w = move forward
s = move backward
a = turn left
d = turn right
. = do nothing
```

A typical manual turn looks like this:

```text
action d
next 2 2 1
rays
dist 2 2 2 2 2 1 1 1 1 1 1 3 3 3 3 3
frame
validate
commit
```

Meaning:

1. `action d` says the player wants to turn right.
2. You manually compute the result and enter `next 2 2 1`.
3. `rays` shows the ray directions that need to be evaluated.
4. You manually compute 16 wall distances and enter them using `dist`.
5. `frame` renders the ASCII framebuffer from your memory values.
6. `validate` optionally checks your calculation.
7. `commit` copies `NEXT` into the current player registers.

## Commands

```text
map           show the external memory map
regs          show player/action/NEXT/DIST registers
action X      set action: w, s, a, d, or .
next x y dir  manually write NEXT registers; dir is 0..3
rays          show ray directions for each of the 16 columns
dist 16 nums  manually write DIST[0..15]
frame         render ASCII frame from current DIST[16]
commit        copy NEXT into the current player state, then render
validate      compare your manual values with the hidden reference
hintnext      print the correct NEXT state
hintdist      print the correct DIST[16]
auto          auto-fill NEXT and DIST, then render and commit
help          show command list
quit          exit
```

## The map

The current map is hardcoded in the source:

```text
############
#..........#
#.####.....#
#.#..#..#..#
#.#..#..#..#
#....#.....#
#..###..####
#..........#
############
```

`#` is a wall, `.` is empty space.

The player is drawn as:

```text
^ = facing north
> = facing east
v = facing south
< = facing west
```

## Rendering model

This is intentionally not a real Doom renderer.

To keep the task human-computable, the screen has only 16 columns, and the ray direction for each column is simplified into one of three directions:

```text
columns 0..4    = left diagonal ray
columns 5..10   = center ray
columns 11..15  = right diagonal ray
```

For each column, the human computes:

```text
DIST[column] = number of grid steps from NEXT position to the first wall
```

The program then converts each distance into an ASCII wall slice:

- small distance = tall/dark wall;
- large distance = short/light wall.

## What counts as “human CPU mode”?

The pure mode is:

```text
action -> next -> rays -> dist -> frame -> commit
```

The following commands are only helpers:

```text
validate
hintnext
hintdist
auto
```

Using `auto` turns the program back into a normal tiny game engine, so it is useful for debugging, but not for the main concept.

## Why this exists

People often joke that Doom can run on anything. This project explores a strange edge case:

> Can Doom run on a human brain if the brain does the computation and an external interface acts as memory/display?

For a full Doom engine, this would be theoretically possible but practically absurd. The state, rendering, collision, map traversal, RNG, and enemy AI would require huge amounts of external memory and extremely slow manual computation.

This PoC reduces the idea to the smallest useful form:

- tiny grid map;
- one player;
- no enemies;
- no textures;
- no timing;
- low-resolution ASCII framebuffer;
- manually computed ray distances.

It is still the same philosophical structure: the human performs the computation, while the machine only stores and displays the result.

## Limitations

- Not real Doom.
- No WAD loading.
- No BSP traversal.
- No enemies, doors, items, weapons, or RNG.
- No true perspective-correct renderer.
- No continuous angles; only four cardinal directions.
- Raycasting is simplified to three discrete ray directions.
- The program contains an optional reference implementation for validation.

## Possible next steps

- Add a Qt GUI with editable memory tables.
- Add a visible microinstruction list for each frame.
- Add a VRAM-like framebuffer editor.
- Add a step-by-step “manual ALU” mode.
- Add enemies with manually updated state.
- Add a small WAD-like level format.
- Add a checksum so the user can prove a frame was computed manually.
- Add “assisted human CPU” mode where the program only checks arithmetic but never computes the next frame.
- Increase the renderer from 16 columns to 32 or 64 columns.
- Add diagonal movement and more view angles.

## Files

```text
human_cpu_doom_poc.cpp   single-file C++17 implementation
README.md                this file
```

## License

MIT, or any permissive license you prefer. Replace this section with your chosen license before publishing.
