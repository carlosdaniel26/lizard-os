# doomgeneric, ported to lizard-os

The port is a **patch over upstream**, not a vendored copy. This directory only
holds:

| file | what |
| --- | --- |
| `doomgeneric.patch` | the whole port: adds `dg_lizard.c` (platform layer + `main`) and `Makefile.lizard` inside upstream's `doomgeneric/` |
| `Makefile` | bootstrap: clone `ozkl/doomgeneric` at the pinned commit into `doomgeneric/` (gitignored), `git apply` the patch, build |
| `wad/` | drop `doom1.wad` here (gitignored); otherwise `makefiles/emulation.mk` fetches Freedoom |

`make` (from the repo root, via `make run`) does all of it. **The first build
needs network** to clone doomgeneric; after that it is offline. `make -C
userspace/doom clean` removes the clone.

## Running

```
make run            # build kernel + userspace (clones doomgeneric) + boot QEMU
```

At the lizard shell: `doom`. Arrows move, Ctrl fires, Space uses doors, Esc =
menu, Enter selects.

## How it hangs together

Platform hook (`dg_lizard.c`)   | lizard mechanism
------------------------------- | -------------------------------------------------
`DG_DrawFrame` (640x400 XRGB)   | `SYS_fb_blit` -> `fb_present_xrgb()` centres + 2x integer-scales into the GOP framebuffer
`DG_GetKey`                     | `SYS_key_get` returns raw set-1 scancodes from a kernel ring buffer; `scancode_to_doom()` translates. First call flips the keyboard to "raw" mode (the shell's line editor stops seeing keys); task exit flips it back.
`DG_GetTicksMs` / `DG_SleepMs`  | `SYS_uptime_ms` / `SYS_sleep`
`fopen`/`fread`/`fseek` on the WAD | freestanding libc in `../lib` -> `SYS_open`/`SYS_read`/`SYS_lseek` -> VFS/FAT16
`malloc` (6 MiB zone + screen)  | 16 MiB static arena in `../lib/stdlib.c`, first-fit

Kernel-side changes this port needed (all in the main tree, not the patch): 7
new syscalls (`abi/syscall.h`), a per-task fd table, a keyboard scancode ring,
`fb_present_xrgb()`, an in-RAM FAT cache (so multi-MB reads aren't one PIO
sector per cluster step), a streaming ELF loader (no whole-file `kmalloc`),
zeroed `.bss` on load, and a bigger user stack.

## Changing the port

Edit inside the clone, then regenerate the patch:

```
cd userspace/doom/doomgeneric
# edit doomgeneric/dg_lizard.c or doomgeneric/Makefile.lizard
git add -f doomgeneric/dg_lizard.c doomgeneric/Makefile.lizard
git diff --cached > ../doomgeneric.patch
```

Bump `DG_COMMIT` in `Makefile` to move to a newer upstream (and refresh the
patch against it).
