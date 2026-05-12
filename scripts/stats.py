#!/usr/bin/env python3
import os
import subprocess
import datetime
import matplotlib.pyplot as plt
import sys
import json

# === Config ===
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.join(SCRIPT_DIR, "../")
SRC_PATH = os.path.join(ROOT_DIR, "kernel/src")
SCRIPTS_PATH = os.path.join(ROOT_DIR, "scripts")
OUTPUT_PNG = "/tmp/project_stats.png"
FALLBACK_FONT = "Liberation Serif"
MODERN_HASH = "7115b36859d11a6b917b6a840ced28d56cbca571"

# -----------------------------
# Helper functions
# -----------------------------
def run_git_cmd(args):
    try:
        return subprocess.check_output(["git"] + args, cwd=ROOT_DIR, text=True).strip()
    except Exception:
        return None

def parse_git_date(date_str):
    if not date_str:
        return None
    try:
        return datetime.datetime.strptime(date_str, "%Y-%m-%d %H:%M:%S %z")
    except ValueError:
        try:
            return datetime.datetime.strptime(date_str, "%a %b %d %H:%M:%S %Y %z")
        except Exception:
            return None

def run_cloc(path, langs):
    try:
        out = subprocess.check_output(
            ["cloc", path, "--json", f"--include-lang={langs}"],
            text=True
        )
        return json.loads(out)
    except Exception:
        return {}

def gather_cloc_stats():
    # Kernel Source Stats
    kernel_data = run_cloc(SRC_PATH, "C,C/C++ Header,Assembly")
    c_stats = kernel_data.get("C", {})
    h_stats = kernel_data.get("C/C++ Header", {})
    asm_stats = kernel_data.get("Assembly", {})

    # Scripts Stats
    scripts_data = run_cloc(SCRIPTS_PATH, "Python,Bourne Shell")
    py_stats = scripts_data.get("Python", {})
    sh_stats = scripts_data.get("Bourne Shell", {})

    stats = {
        "kernel": {
            "c_code": c_stats.get("code", 0),
            "h_code": h_stats.get("code", 0),
            "asm_code": asm_stats.get("code", 0),
            "total_code": c_stats.get("code", 0) + h_stats.get("code", 0) + asm_stats.get("code", 0),
            "comment": c_stats.get("comment", 0) + h_stats.get("comment", 0) + asm_stats.get("comment", 0),
            "blank": c_stats.get("blank", 0) + h_stats.get("blank", 0) + asm_stats.get("blank", 0),
        },
        "scripts": {
            "code": py_stats.get("code", 0) + sh_stats.get("code", 0),
            "comment": py_stats.get("comment", 0) + sh_stats.get("comment", 0),
            "blank": py_stats.get("blank", 0) + sh_stats.get("blank", 0),
        }
    }
    return stats

# -----------------------------
# Gather repo info
# -----------------------------
def gather_repo_info():
    info = {}
    info["total_commits"] = int(run_git_cmd(["rev-list", "--count", "HEAD"]) or 0)

    legacy_hash = run_git_cmd(["rev-list", "--max-parents=0", "HEAD"])
    legacy_date_str = run_git_cmd(["show", "-s", "--format=%ad", "--date=iso", legacy_hash])
    legacy_date = parse_git_date(legacy_date_str)
    info["legacy_creation"] = legacy_date
    now_aware = datetime.datetime.now(datetime.timezone.utc)
    info["legacy_days"] = (now_aware - legacy_date).days if legacy_date else None

    modern_date_str = run_git_cmd(["show", "-s", "--format=%ad", "--date=iso", MODERN_HASH])
    modern_date = parse_git_date(modern_date_str)
    info["modern_creation"] = modern_date
    info["modern_days"] = (now_aware - modern_date).days if modern_date else None

    info["stats"] = gather_cloc_stats()
    return info

# -----------------------------
# Terminal view
# -----------------------------
def print_terminal(info):
    s = info["stats"]
    print("="*50)
    print("Project Dashboard (Terminal View)")
    print("="*50)
    print(f"Total commits         : {info.get('total_commits','N/A')}")
    print(f"Legacy creation       : {info['legacy_creation'].date() if info['legacy_creation'] else 'N/A'} (Days: {info.get('legacy_days','N/A')})")
    print(f"Modern creation       : {info['modern_creation'].date() if info['modern_creation'] else 'N/A'} (Days: {info.get('modern_days','N/A')})")
    print("-" * 50)
    print("Kernel Source Statistics:")
    print(f"  C code lines        : {s['kernel']['c_code']}")
    print(f"  C Header lines      : {s['kernel']['h_code']}")
    print(f"  Assembly code lines : {s['kernel']['asm_code']}")
    print(f"  Total Kernel Code   : {s['kernel']['total_code']}")
    print(f"  Total Kernel Comment: {s['kernel']['comment']}")
    print("-" * 50)
    print("Scripts Statistics:")
    print(f"  Total Scripts Code  : {s['scripts']['code']}")
    print(f"  Total Scripts Comm. : {s['scripts']['comment']}")
    print("="*50)

# -----------------------------
# Picture view (pretty)
# -----------------------------
def draw_dashboard(info, output_file=OUTPUT_PNG):
    fontname = "Times New Roman"
    import matplotlib.font_manager as fm
    if not any("Times" in f.name for f in fm.fontManager.ttflist):
        fontname = FALLBACK_FONT

    s = info["stats"]
    lines = [
        f"Project Dashboard",
        "",
        f"Total commits         : {info.get('total_commits','N/A')}",
        f"Legacy creation       : {info['legacy_creation'].date() if info['legacy_creation'] else 'N/A'} (Days: {info.get('legacy_days','N/A')})",
        f"Modern creation       : {info['modern_creation'].date() if info['modern_creation'] else 'N/A'} (Days: {info.get('modern_days','N/A')})",
        "",
        f"Kernel Source:",
        f"  C code lines        : {s['kernel']['c_code']}",
        f"  C Header lines      : {s['kernel']['h_code']}",
        f"  Assembly code lines : {s['kernel']['asm_code']}",
        f"  Total Kernel Code   : {s['kernel']['total_code']}",
        f"  Total Kernel Comment: {s['kernel']['comment']}",
        "",
        f"Scripts:",
        f"  Total Scripts Code  : {s['scripts']['code']}",
        f"  Total Scripts Comm. : {s['scripts']['comment']}"
    ]

    fig_height = len(lines) * 0.5
    fig, ax = plt.subplots(figsize=(10, fig_height))
    ax.axis("off")

    fig.suptitle("Project Dashboard", fontsize=24, fontweight='bold', fontname=fontname, y=0.98)

    y_pos = 0.92
    for i, line in enumerate(lines[2:]):
        is_sub = line.startswith("  ")
        is_header = line.endswith(":")
        fsize = 14 if is_sub else 16
        fweight = 'bold' if is_header else 'normal'
        color = "#34495e" if is_sub else "#2c3e50"
        
        ax.text(0.05, y_pos, line, fontsize=fsize, fontweight=fweight, fontname=fontname, va='top', ha='left', color=color, transform=ax.transAxes)
        y_pos -= 0.06

    plt.tight_layout()
    plt.savefig(output_file, dpi=150)
    print(f"Dashboard saved to {output_file}")
    try:
        subprocess.run(["xdg-open", output_file], check=False)
    except Exception:
        pass

# -----------------------------
# Main
# -----------------------------
if __name__=="__main__":
    info = gather_repo_info()
    view = "terminal"
    if len(sys.argv) > 1 and sys.argv[1] == "--view":
        if len(sys.argv) > 2 and sys.argv[2] == "pic":
            view = "pic"

    if view == "pic":
        draw_dashboard(info)
    else:
        print_terminal(info)
