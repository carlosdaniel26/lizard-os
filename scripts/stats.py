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

def run_cloc():
    try:
        out = subprocess.check_output(
            ["cloc", SRC_PATH, "--json", "--include-lang=C,Header,Assembly"],
            text=True
        )
        data = json.loads(out)
        total_code = sum([v.get("code",0) for k,v in data.items() if isinstance(v, dict)])
        total_comment = sum([v.get("comment",0) for k,v in data.items() if isinstance(v, dict)])
        total_blank = sum([v.get("blank",0) for k,v in data.items() if isinstance(v, dict)])
        return total_code, total_comment, total_blank
    except Exception:
        return "N/A","N/A","N/A"

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

    code, comment, blank = run_cloc()
    info["cloc_summary"] = {"code": code, "comment": comment, "blank": blank}
    return info

# -----------------------------
# Terminal view
# -----------------------------
def print_terminal(info):
    print("="*50)
    print("Project Dashboard (Terminal View)")
    print("="*50)
    print(f"Total commits         : {info.get('total_commits','N/A')}")
    print(f"Legacy creation       : {info['legacy_creation'].date() if info['legacy_creation'] else 'N/A'} (Days: {info.get('legacy_days','N/A')})")
    print(f"Modern creation       : {info['modern_creation'].date() if info['modern_creation'] else 'N/A'} (Days: {info.get('modern_days','N/A')})")
    print(f"Total lines of code   : {info['cloc_summary'].get('code','N/A')}")
    print(f"Total comment lines   : {info['cloc_summary'].get('comment','N/A')}")
    print(f"Total blank lines     : {info['cloc_summary'].get('blank','N/A')}")
    print("="*50)

# -----------------------------
# Picture view (pretty)
# -----------------------------
def draw_dashboard(info, output_file=OUTPUT_PNG):
    fontname = "Times New Roman"
    import matplotlib.font_manager as fm
    if not any("Times" in f.name for f in fm.fontManager.ttflist):
        fontname = FALLBACK_FONT

    lines = [
        f"Project Dashboard",
        "",
        f"Total commits         : {info.get('total_commits','N/A')}",
        f"Legacy creation       : {info['legacy_creation'].date() if info['legacy_creation'] else 'N/A'} (Days: {info.get('legacy_days','N/A')})",
        f"Modern creation       : {info['modern_creation'].date() if info['modern_creation'] else 'N/A'} (Days: {info.get('modern_days','N/A')})",
        "",
        f"Total lines of code   : {info['cloc_summary'].get('code','N/A')}",
        f"Total comment lines   : {info['cloc_summary'].get('comment','N/A')}",
        f"Total blank lines     : {info['cloc_summary'].get('blank','N/A')}"
    ]

    fig_height = len(lines) * 0.7
    fig, ax = plt.subplots(figsize=(10, fig_height))
    ax.axis("off")

    fig.suptitle("Project Dashboard", fontsize=28, fontweight='bold', fontname=fontname, y=0.95)

    y_pos = 0.85
    for i, line in enumerate(lines[2:]):  # skip the main title in lines
        ax.text(0.05, y_pos, line, fontsize=16, fontname=fontname, va='top', ha='left', color="#2c3e50", transform=ax.transAxes)
        y_pos -= 0.08

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