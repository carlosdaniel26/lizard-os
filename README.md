<p align="center">
  <img src="https://github.com/user-attachments/assets/33c7f305-a435-4b75-8c22-39f9e8cdc589" alt="lizard-os-logo" width="400" height="400">
</p>

<h1 align="center">Lizard-OS</h1>

<p align="center">
  Lizard-OS is a toy operating system. Not a product. Not a clone. Just raw low-level computing the way it should be: built from scratch, for learning and control.  
</p>

---

## ⚙️ About

Lizard-OS is a hobby kernel and libc written by someone who got tired of black boxes. Inspired by books like *Linux: Just for Fun*, *Computer Systems: A Programmer’s Perspective*, and *Operating Systems: Three Easy Pieces*, this project is where abstraction goes to die — in the best way possible.

It’s not production-ready. It’s not POSIX-compliant. It exists to teach and tinker with the internals of CPUs, memory, filesystems, and hardware protocols.

If you're here to understand how systems work — not how to use systems that already work — welcome.

---

## 🐍 Features (So Far)

- **Multiboot2-compliant bootloader support**  
  `process_multiboot2_tags()` gets all the low-level boot info right after GRUB hands over control.

- **Basic libc-like utilities**  
  Forget glibc. You’ve got:
  - `kprint()`, `kprintf()`, `putchar()` for raw output
  - Custom string functions like `strlen`, `strcmp`, `strsIsEqual`
  - `unsigned_to_string()` and `unsigned_to_hexstring()` for dirty debugging

- **Memory Management**  
  - Physical memory manager (`pmm_init()`, `pmm_reserve_block()`)
  - Paging and early heap (`kmalloc(bytes)`, `kfree()`, `enable_paging()`)

- **Multitasking**  
  - PID allocator, task switching, context saving
  - Cooperative scheduler
  - Functions like `block_task()`, `terminate_task()`, `switch_task()`

- **I/O and Devices**
  - **ATA PIO driver**: `atapio_read_sector()`, `atapio_write_sector()`
  - **RAM Disk** for testing without real disks
  - **Keyboard, PIT, RTC, PIC**, and **basic IDT/GDT setup**
  - Low-level I/O: `inb()`, `outb()`, `io_wait()`, etc.

- **File and VFS layer (WIP)**  
  Basic file abstraction: `vfs_read()`, `vfs_write()`, `vfs_delete()`  
  FAT16 test read to verify BPB and raw file access.

- **Terminal and Shell**  
  - Text-mode framebuffer driver: `draw_char()`, `scroll_framebuffer()`
  - TTY handler with input handling
  - Dumb shell: `shit_shell_init()`, `runcmd()`  
    Yes, that’s its actual name. It works, get over it.

- **Debugging**
  - Use `dd()` or `debug_printf()` to spit out what you need when you need it.
  - `kprint_task_state()` to inspect tasks.

---

## 💾 Build and Run

```sh
make dev
```
