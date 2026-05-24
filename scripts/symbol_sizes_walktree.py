#!/usr/bin/env python3
import os
import curses
import subprocess
from collections import defaultdict

def format_size(size):
    if size < 1024:
        return f"{size} B"
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
        
        cmd_line = ["addr2line", "-e", binary, addr]
        output = subprocess.run(cmd_line, capture_output=True, text=True).stdout.strip()
        
        if name.startswith("__initcall_"):
            path = "initcall/registry.c"
        elif output.startswith("??:"):
            path = "unknown/unknown.c"
        else:
            path = output.split(':')[0]
            if path.startswith(project_root):
                path = os.path.relpath(path, project_root)
        
        symbol_map[path].append({'name': name, 'size': size})
    return symbol_map

class Node:
    def __init__(self, name, parent=None):
        self.name = name
        self.parent = parent
        self.children = {}
        self.symbols = []
        self.total_size = 0

    def add_symbol(self, path_parts, symbol):
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
        self.children[part].add_symbol(path_parts[1:], symbol)

def get_flat_nodes(node, depth, expanded_nodes, sort_key, reverse):
    res = [(node, depth)]
    if node in expanded_nodes:
        # Sort children and symbols locally
        children = sorted(node.children.values(), 
                          key=lambda n: n.total_size if sort_key == 'size' else n.name, 
                          reverse=reverse)
        symbols = sorted(node.symbols, 
                         key=lambda s: s[sort_key], 
                         reverse=reverse)
        
        for child in children:
            res.extend(get_flat_nodes(child, depth + 1, expanded_nodes, sort_key, reverse))
        for sym in symbols:
            res.append((sym, depth + 1))
    return res

def draw(stdscr, nodes, selected_index, scroll_offset, sort_mode):
    stdscr.clear()
    h, w = stdscr.getmaxyx()
    for i, (item, depth) in enumerate(nodes):
        if i < scroll_offset or i >= scroll_offset + h - 1: continue
        prefix = "  " * depth
        if isinstance(item, Node):
            label = f"{prefix}{'[-] ' if item in expanded_nodes else '[+] '}{item.name} ({format_size(item.total_size)})"
        else:
            label = f"{prefix}  * {item['name']} ({format_size(item['size'])})"
        
        if i == selected_index:
            stdscr.attron(curses.A_REVERSE)
            stdscr.addnstr(i - scroll_offset, 0, label, w - 1)
            stdscr.attroff(curses.A_REVERSE)
        else:
            stdscr.addnstr(i - scroll_offset, 0, label, w - 1)
    
    status = f" Ordering by: {sort_mode} | 'o' to change | 'q' to quit"
    stdscr.addstr(h-1, 0, status[:w-1], curses.A_BOLD)
    stdscr.refresh()

expanded_nodes = set()

def main(stdscr):
    curses.curs_set(0)
    symbol_map = get_symbol_map()
    root = Node("root")
    for path, syms in symbol_map.items():
        parts = path.split(os.sep)
        for sym in syms:
            root.add_symbol(parts, sym)
    
    global expanded_nodes
    expanded_nodes.add(root)
    
    sort_opts = [('size', True, "Size ^"), ('size', False, "Size v"), ('name', False, "Name ^"), ('name', True, "Name v")]
    sort_idx = 0
    selected, scroll = 0, 0
    
    while True:
        key, rev, label = sort_opts[sort_idx]
        nodes = get_flat_nodes(root, 0, expanded_nodes, key, rev)
        
        draw(stdscr, nodes, selected, scroll, label)
        c = stdscr.getch()
        
        if c == ord('q'): break
        elif c == ord('o'): sort_idx = (sort_idx + 1) % len(sort_opts)
        elif c == curses.KEY_UP and selected > 0: selected -= 1
        elif c == curses.KEY_DOWN and selected < len(nodes) - 1: selected += 1
        elif c in [ord('\n'), curses.KEY_RIGHT]:
            item = nodes[selected][0]
            if isinstance(item, Node): expanded_nodes.add(item)
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
    curses.wrapper(main)
