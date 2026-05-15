#!/usr/bin/env python3
import subprocess
import sys
import os
import re

def run_command(cmd):
    result = subprocess.run(cmd, capture_output=True, text=True, shell=True)
    if result.returncode != 0:
        return None
    return result.stdout

def parse_sections(binary):
    output = run_command(f"readelf -S {binary}")
    if not output: return []
    
    sections = []
    # Readelf -S output is often split into two lines
    # [ 1] .text             PROGBITS         ffffffff80200000  00001000
    #      0000000000001000  0000000000000000  AX       0     0     16
    
    lines = output.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        match = re.search(r"\[\s*(\d+)\]\s+([\.\w\-_]+)\s+\w+\s+([0-9a-f]+)\s+([0-9a-f]+)", line)
        if match:
            idx, name, addr, off = match.groups()
            # The size is on the next line usually
            i += 1
            if i < len(lines):
                size_match = re.search(r"([0-9a-f]+)\s+([0-9a-f]+)", lines[i].strip())
                if size_match:
                    size = size_match.group(1)
                    sections.append({
                        "idx": int(idx),
                        "name": name,
                        "addr": int(addr, 16),
                        "off": int(off, 16),
                        "size": int(size, 16)
                    })
        i += 1
    return sections

def parse_symbols(binary):
    output = run_command(f"nm -S --numeric-sort {binary}")
    if not output: return []
    
    symbols = []
    # Match lines like: ffffffff80200000 0000000000000001 T kmain
    pattern = re.compile(r"([0-9a-f]+)\s+([0-9a-f]+)?\s+(\w)\s+(\w+)")
    for line in output.splitlines():
        match = pattern.search(line)
        if match:
            addr, size, type, name = match.groups()
            symbols.append({
                "addr": int(addr, 16),
                "size": int(size, 16) if size else 0,
                "type": type,
                "name": name
            })
    return symbols

def generate_map(binary, output_path):
    sections = parse_sections(binary)
    symbols = parse_symbols(binary)
    
    if not sections:
        print(f"Failed to parse sections from {binary}")
        return

    with open(output_path, "w") as f:
        f.write(f"Kernel Memory Map: {binary}\n")
        f.write("=" * 60 + "\n\n")
        
        f.write("Sections:\n")
        f.write(f"{'Name':<20} {'Address':<18} {'Size':<10} {'End':<18}\n")
        f.write("-" * 66 + "\n")
        for s in sections:
            if s['addr'] == 0: continue
            end = s['addr'] + s['size']
            f.write(f"{s['name']:<20} 0x{s['addr']:016x} 0x{s['size']:08x} 0x{end:016x}\n")
            
        f.write("\nSignificant Symbols:\n")
        f.write(f"{'Name':<30} {'Address':<18} {'Size':<10}\n")
        f.write("-" * 58 + "\n")
        
        targets = ["kernel_start", "kernel_end", "kernel_stack", "kpml4", "hhdm_offset"]
        for sym in symbols:
            if sym['name'] in targets or sym['type'].upper() in 'T':
                if sym['name'] in targets or sym['size'] > 1024:
                    f.write(f"{sym['name']:<30} 0x{sym['addr']:016x} 0x{sym['size']:08x}\n")

    print(f"Map generated at {output_path}")

if __name__ == "__main__":
    binary = "build/x86_64/kernel/x86_64/kernel"
    if len(sys.argv) > 1:
        binary = sys.argv[1]
    
    output = "kernel.map.txt"
    if os.path.exists(binary):
        generate_map(binary, output)
    else:
        # Try to find it if relative path fails
        found = False
        for root, dirs, files in os.walk('.'):
            if 'kernel' in files and 'build' in root:
                binary = os.path.join(root, 'kernel')
                generate_map(binary, output)
                found = True
                break
        if not found:
            print(f"Binary {binary} not found.")
