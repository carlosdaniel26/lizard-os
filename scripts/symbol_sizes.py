#!/usr/bin/env python3
import os
import curses
import subprocess
from collections import defaultdict
import argparse

def format_size(size):
    if size < 1024: return f"{size} B"
    return f"{size / 1024:.1f} KiB ({size} B)"

def get_symbol_map():
    binary = "build/bin/kernel"
    cmd = ["nm", "--print-size", "--size-sort", "--radix=x", binary]
    res = subprocess.run(cmd, capture_output=True, text=True).stdout
    symbol_map = defaultdict(list)
    project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    for line in res.splitlines():
        parts = line.split()
        if len(parts) < 4: continue
        size = int(parts[1], 16)
        name = parts[3]
        addr = parts[0]
        stype = parts[2]
        output = subprocess.run(["addr2line", "-e", binary, addr], capture_output=True, text=True).stdout.strip()
        path = "initcall/registry.c" if name.startswith("__initcall_") else ("unknown/unknown.c" if output.startswith("??:") else output.split(':')[0])
        if path.startswith(project_root): path = os.path.relpath(path, project_root)
        symbol_map[path].append({'name': name, 'size': size, 'stype': stype})
    return symbol_map

class Node:
    def __init__(self, name, parent=None):
        self.name = name
        self.parent = parent
        self.children = {}
        self.symbols = []
        self.total_size = 0
        self.sort_idx = 0

    def toggle_sort(self):
        self.sort_idx = (self.sort_idx + 1) % 4

    def add_symbol(self, path_parts, symbol, bssonly, nobss):
        # Filtering logic
        stype = symbol['stype'].lower()
        if nobss and stype == 'b': return
        if bssonly and stype != 'b': return
            
        self.total_size += symbol['size']
        if not path_parts:
            for s in self.symbols:
                if s['name'] == symbol['name']:
                    s['size'] += symbol['size']
                    return
            self.symbols.append(symbol)
            return
        part = path_parts[0]
        if part not in self.children: self.children[part] = Node(part, parent=self)
        self.children[part].add_symbol(path_parts[1:], symbol, bssonly, nobss)

def get_flat_nodes(node, depth, expanded_nodes):
    res = [(node, depth)]
    if node in expanded_nodes:
        sort_opts = [('size', True, "Size ^"), ('size', False, "Size v"), ('name', False, "Name ^"), ('name', True, "Name v")]
        key, reverse, _ = sort_opts[node.sort_idx]
        children = sorted(node.children.values(), 
                          key=lambda n: n.total_size if key == 'size' else n.name, 
                          reverse=reverse)
        symbols = sorted(node.symbols, 
                         key=lambda s: s[key], 
                         reverse=reverse)
        
        for child in children: res.extend(get_flat_nodes(child, depth + 1, expanded_nodes))
        for sym in symbols: res.append((sym, depth + 1))
    return res

def draw(stdscr, nodes, selected_index, scroll_offset, expanded_nodes):
    stdscr.clear()
    h, w = stdscr.getmaxyx()
    for i, (item, depth) in enumerate(nodes):
        if i < scroll_offset or i >= scroll_offset + h - 1: continue
        prefix = "  " * depth
        if isinstance(item, Node):
            label = f"{prefix}{'[-] ' if item in expanded_nodes else '[+] '}{item.name} ({format_size(item.total_size)})"
            sort_opts = ["Size ^", "Size v", "Name ^", "Name v"]
            if item in expanded_nodes: label += f" [Ordering by: {sort_opts[item.sort_idx]}]"
        else:
            label = f"{prefix}  * {item['name']} ({format_size(item['size'])})"
        
        if i == selected_index:
            stdscr.attron(curses.A_REVERSE)
            stdscr.addnstr(i - scroll_offset, 0, label, w - 1)
            stdscr.attroff(curses.A_REVERSE)
        else:
            stdscr.addnstr(i - scroll_offset, 0, label, w - 1)
    stdscr.addstr(h-1, 0, " 'o' sort | 'Enter'/'Right' expand/toggle | 'Left' collapse/parent | 'q' quit", curses.A_BOLD)
    stdscr.refresh()

def main(stdscr, bssonly, nobss):
    curses.curs_set(0)
    symbol_map = get_symbol_map()
    root = Node("root")
    for path, syms in symbol_map.items():
        parts = path.split(os.sep)
        for sym in syms: root.add_symbol(parts, sym, bssonly, nobss)
    
    expanded_nodes = {root}
    selected, scroll = 0, 0
    while True:
        nodes = get_flat_nodes(root, 0, expanded_nodes)
        draw(stdscr, nodes, selected, scroll, expanded_nodes)
        c = stdscr.getch()
        
        if c == ord('q'): break
        elif c == ord('o'):
            item = nodes[selected][0]
            if isinstance(item, Node): item.toggle_sort()
        elif c == curses.KEY_UP and selected > 0: selected -= 1
        elif c == curses.KEY_DOWN and selected < len(nodes) - 1: selected += 1
        elif c in [ord('\n'), curses.KEY_RIGHT]:
            item = nodes[selected][0]
            if isinstance(item, Node):
                if item in expanded_nodes: expanded_nodes.remove(item)
                else: expanded_nodes.add(item)
        elif c == curses.KEY_LEFT:
            item = nodes[selected][0]
            if isinstance(item, Node) and item in expanded_nodes: expanded_nodes.remove(item)
            elif item.parent:
                for i, (n, _) in enumerate(nodes):
                    if n == item.parent: selected = i; break
        h, _ = stdscr.getmaxyx()
        if selected < scroll: scroll = selected
        if selected >= scroll + h - 1: scroll = selected - h + 2

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Interactive kernel symbol size analyzer.")
    parser.add_argument("--nobss", action="store_true", help="Exclude symbols from the .bss section (runtime memory).")
    parser.add_argument("--bssonly", action="store_true", help="Show only symbols from the .bss section.")
    args = parser.parse_args()
    curses.wrapper(lambda stdscr: main(stdscr, args.bssonly, args.nobss))
