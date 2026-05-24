#!/usr/bin/env python3
import os

def hex_to_ansi(hex_color):
    hex_color = hex_color.lstrip('#')
    r, g, b = tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
    return f"\033[38;2;{r};{g};{b}m"

def format_size(path):
    if not os.path.exists(path): return "N/A"
    size = os.path.getsize(path)
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size < 1024: return f"{size:.1f}{unit}".replace('.0', '')
        size /= 1024
    return f"{size:.1f}T"

def main():
    RESET = "\033[0m"
    GREEN = hex_to_ansi("#26a269")
    BRIGHT_YELLOW = hex_to_ansi("#ffffaf")
    print(f"{GREEN}Compilation Succeed{RESET}")
    print(f"{BRIGHT_YELLOW}Kernel size:{RESET} {format_size('build/bin/kernel')}  "
          f"{BRIGHT_YELLOW}ISO size:{RESET} {format_size('lizard-os_x86_64.iso')}")

if __name__ == "__main__":
    main()
