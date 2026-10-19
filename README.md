# GoWR 4:3 Patch

True 4:3 for **God of War Ragnarök (PC, Steam)**: no black bars and no stretching.

## Download

**[⬇ Download the latest release](https://github.com/Drilon01/4x3-Aspect-Ratio-GoWr/releases/latest)**: get
`GoWR-4x3-Patch-main.zip` under *Assets*. It contains the patcher, the undo tool,
a step-by-step guide and the license.

## The problem

In *Auto* mode the game snaps the screen to the nearest entry of a built-in
aspect-ratio table: 16:9, 16:10, 17:9, 21:9, 21.5:9 and 32:9. There is no 4:3
entry, so a 4:3 screen snaps to 16:10. A 2048×1536 display ends up rendering
2048×1280, with black bars at the top and bottom.

## The fix

The patcher replaces the **16:10** entry with **4:3**. 11 bytes change in total,
all in read-only data (`.rdata`) and none in code:

| Table | Original | Patched | File offset* |
|---|---|---|---|
| width : height pairs (float) | 16.0 : 10.0 | 4.0 : 3.0 | `0x1D643C8` |
| aspect ratio (float) | 1.6 | 1.33333 | `0x1D643F8` |
| inverse ratio (float) | 0.625 | 0.75 | `0x1D64418` |
| menu label (string) | `16:10` | `4:3` | `0x1D64474` |

\* Offsets are for GoWR.exe with SHA-256
`4d242ab5577379418196c692d3dff989f7223d3d7d01e8e0d2736cbc512c0eff`
(46,685,144 bytes). The patcher does **not** use fixed offsets. It locates each
table by a byte pattern that includes the neighbouring entries, and it only
patches when every pattern matches exactly once. A different game version is
refused and left unchanged.

What the patcher does:
- Saves `GoWR.exe.4x3-backup` before the first change.
- Asks for administrator rights, because the game lives in Program Files.
- Reads and writes only `GoWR.exe` in its own folder. It makes no network
  calls and changes nothing in the registry.

`GoWR43-Undo.exe` is built from the same source with `-DUNDO_BUILD` and
restores the original bytes.

## Install

1. Close the game.
2. Copy `GoWR43-Patcher.exe` and `GoWR43-Undo.exe` next to `GoWR.exe`.
   To find the game folder: Steam → right-click the game → Manage → Browse local files.
3. Run `GoWR43-Patcher.exe` once.
4. In the game, set **Aspect Ratio = Auto** (or "4:3", which replaces "16:10" in the list).

To undo, run `GoWR43-Undo.exe`, or use Steam → Verify integrity of game files.

**Who it's for:** 4:3 screens and 4:3 resolutions. 5:4 and 3:2 screens
also snap to 4:3. **Don't use it on 16:10 monitors**, because the 16:10 option is
removed.

## Build from source

The release binaries are built with [Zig](https://ziglang.org/) 0.17.0 as the
C compiler (`zig cc`). It's one download and needs no Visual Studio.

**Windows:** install Zig (`winget install zig.zig`, or
`pip install ziglang==0.17.0`), then run:

```
build.bat
```

**Linux / macOS** (cross-compile):

```
./build.sh
```

Both scripts produce `GoWR43-Patcher.exe` and `GoWR43-Undo.exe` in `bin/`.
The commands are:

```
zig cc -target x86_64-windows-gnu -O2 -s src/gowr43patch.c -lshell32 -ladvapi32 -o bin/GoWR43-Patcher.exe
zig cc -target x86_64-windows-gnu -O2 -s -DUNDO_BUILD src/gowr43patch.c -lshell32 -ladvapi32 -o bin/GoWR43-Undo.exe
```

## Verify the release

Building with Zig 0.17.0 is reproducible, so your build should match the
released binaries byte for byte:

| File | SHA-256 |
|---|---|
| GoWR43-Patcher.exe | `5bbd31ede08ce1bb7e1e0043c05487581e00d1c9ea429d48e1fea8ab91dac00e` |
| GoWR43-Undo.exe | `b849f4921505643fad752b333e8b6e7b17d9d6457475a8131a3176e44dae2284` |

## License

© 2026 Driloni. Licensed under
[CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/).

- ✅ You may use, share and modify this work.
- ✅ **Credit required.** Name Driloni as the original author and link back here.
  Don't present this work, or a modified version of it, as your own.
- ❌ **No commercial use.** Don't sell it, paywall it or bundle it into a paid product.
- 🔁 **Share-alike.** Modified versions must be released under the same license.

The full license text is in [LICENSE](LICENSE).
