#!/usr/bin/env python3
import re
import os
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch
from collections import defaultdict
import subprocess

# === Config ===
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.join(SCRIPT_DIR, "../kernel/src")
OUTPUT_PNG = "/tmp/initcall_blocks.png"

PATTERN = re.compile(
    r'([a-z_]+_initcall(?:_prio)?)\s*\(\s*([a-zA-Z_]\w*)\s*(?:,\s*([0-9]+)\s*)?\)'
)

LEVEL_MAP = {
    "early_initcall": 0,
    "memory_initcall": 1,
    "memory_initcall_prio": 1,
    "core_initcall": 2,
    "postcore_initcall": 2.1,
    "arch_initcall": 3,
    "fs_initcall": 5,
    "device_initcall": 6,
    "late_initcall": 7,
}

LEVEL_NAMES = {
    0: "early_initcall",
    1: "memory_initcall",
    2: "core_initcall",
    2.1: "postcore_initcall",
    3: "arch_initcall",
    5: "fs_initcall",
    6: "device_initcall",
    7: "late_initcall",
    99: "unknown"
}

# Importance color palette: cool → warm → urgent
LEVEL_COLORS = {
    0: "#d4e6f1",   # early_initcall – light blue
    1: "#a9cce3",   # memory_initcall – soft blue
    2: "#f9e79f",   # core_initcall – soft yellow
    2.1: "#f7dc6f", # postcore_initcall – deeper yellow
    3: "#f5b041",   # arch_initcall – orange
    5: "#f1948a",   # fs_initcall – soft red
    6: "#e74c3c",   # device_initcall – strong red
    7: "#922b21",   # late_initcall – dark red
    99: "#bfbfbf"   # unknown – gray
}

# Fallback font
FALLBACK_FONT = "Liberation Serif"

def parse_initcalls(root_dir):
    results = []
    for root, _, files in os.walk(root_dir):
        for file in files:
            if not file.endswith(".c"):
                continue
            path = os.path.join(root, file)
            try:
                with open(path, "r", errors="ignore") as f:
                    for lineno, line in enumerate(f, 1):
                        for match in PATTERN.finditer(line):
                            macro, fn, prio = match.groups()
                            lvl = LEVEL_MAP.get(macro, 99)
                            prio_val = int(prio) if prio else 0
                            rel = os.path.relpath(path, ROOT_DIR)
                            results.append({
                                "fn": fn,
                                "lvl": lvl,
                                "sub": prio_val,
                                "file": f"{rel}:{lineno}"
                            })
            except Exception:
                continue
    return results

def draw_blocks(results, output_file=OUTPUT_PNG):
    levels = defaultdict(list)
    for r in results:
        levels[r["lvl"]].append(r)

    sorted_levels = sorted(levels.keys())
    if not sorted_levels:
        print("No initcalls to display.")
        return

    # Sizes & spacing
    level_height = 80
    node_height = 60
    node_spacing = 30
    margin = 30
    level_fontsize = 20
    node_fontsize = 16

    # Determine font
    fontname = "Times New Roman"
    import matplotlib.font_manager as fm
    if not any("Times" in f.name for f in fm.fontManager.ttflist):
        fontname = FALLBACK_FONT

    fig, ax = plt.subplots(figsize=(12, len(sorted_levels)*2))

    # Add title
    fig.suptitle("Lizard-OS Kernel Initcall Levels", fontsize=28, fontweight='bold', fontname=fontname, y=0.98)

    # Measure dynamic block widths
    max_width = 0
    for lvl in sorted_levels:
        x = margin
        for node in levels[lvl]:
            text = node["fn"]
            t = ax.text(0,0,text, fontsize=node_fontsize, fontname=fontname)
            fig.canvas.draw()
            renderer = fig.canvas.get_renderer()
            bbox = t.get_window_extent(renderer=renderer)
            text_width = bbox.width / fig.dpi * 100
            t.remove()
            block_width = max(text_width + 40, 80)
            x += block_width + node_spacing
        max_width = max(max_width, x)

    width = max_width + margin
    height = len(sorted_levels)*(level_height + node_height + margin) + margin
    fig.set_size_inches(width/100, height/100)
    ax.set_xlim(0, width)
    ax.set_ylim(0, height)
    ax.axis("off")

    y = height - margin
    for lvl in sorted_levels:
        nodes = levels[lvl]

        # Draw level label block
        ax.add_patch(FancyBboxPatch((margin, y-level_height), width-2*margin, level_height,
                                    boxstyle="round,pad=0.02", edgecolor='black', facecolor="#eeeeee"))
        ax.text(margin+15, y-level_height/2, f"Level {int(lvl)}: {LEVEL_NAMES.get(lvl)}",
                va='center', ha='left', fontsize=level_fontsize, fontname=fontname, fontweight='bold')

        # Draw initcall blocks
        x = margin
        y_nodes = y - level_height - 10 - node_height
        for node in nodes:
            text = node["fn"]
            t = ax.text(0,0,text, fontsize=node_fontsize, fontname=fontname)
            fig.canvas.draw()
            renderer = fig.canvas.get_renderer()
            bbox = t.get_window_extent(renderer=renderer)
            text_width = bbox.width / fig.dpi * 100
            t.remove()
            block_width = max(text_width + 40, 80)

            ax.add_patch(FancyBboxPatch((x, y_nodes), block_width, node_height,
                                        boxstyle="round,pad=0.02", edgecolor='black',
                                        facecolor=LEVEL_COLORS.get(lvl,"#888888")))
            ax.text(x + block_width/2, y_nodes + node_height/2, text,
                    ha='center', va='center', fontsize=node_fontsize, fontname=fontname)
            x += block_width + node_spacing

        y -= level_height + node_height + margin

    plt.tight_layout()
    plt.savefig(output_file, dpi=150)
    print(f"Tree saved to {output_file}")

    try:
        subprocess.run(["xdg-open", output_file], check=False)
    except Exception:
        pass

if __name__=="__main__":
    results = parse_initcalls(ROOT_DIR)
    draw_blocks(results)
    