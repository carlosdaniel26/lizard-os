#!/usr/bin/env sh
# Split-screen kernel debugging: QEMU (frozen, gdbstub on :1234) in the top tmux
# pane, a debugger attached to it below. Replaces the old two-terminal dance of
# `make debug` in one shell and `make gdb` in another.
#
# Usage: scripts/debug.sh   (normally invoked via `make debug`)
#
# Env:
#   DBG        debugger front-end: ddd | gdb  (default ddd)
#   GDB_PORT   gdbstub tcp port               (default 1234)
#   TMUX_SESS  tmux session name              (default lizard-dbg)
#   GDB_WAIT   seconds to wait for QEMU's port (default 30)
#   NO_TMUX    if set, just run the halted QEMU (no tmux, no debugger)
#
# The connect/break/continue commands (and `source scripts/kheap.py`, the
# buddy/slab/kmalloc tracker) live in script.gdb, so both front-ends and a
# hand-run `gdb -x script.gdb` behave identically.

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

[ -n "${NO_TMUX:-}" ] && exec make _debug-qemu

DBG=${DBG:-ddd}
GDB_PORT=${GDB_PORT:-1234}
TMUX_SESS=${TMUX_SESS:-lizard-dbg}
GDB_WAIT=${GDB_WAIT:-30}

command -v tmux >/dev/null 2>&1 || { echo "error: tmux not found - sudo apt install tmux" >&2; exit 1; }

case "$DBG" in
    ddd)
        command -v ddd >/dev/null 2>&1 || { echo "error: ddd not found - sudo apt install ddd" >&2; exit 1; }
        [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ] || { echo "error: no X display for ddd - set DBG=gdb for a terminal debugger" >&2; exit 1; }
        DBG_LAUNCH="ddd --debugger 'gdb -x $ROOT/script.gdb' $ROOT/build/bin/kernel"
        ;;
    gdb)
        command -v gdb >/dev/null 2>&1 || { echo "error: gdb not found" >&2; exit 1; }
        DBG_LAUNCH="gdb -tui -x script.gdb"
        ;;
    *)
        echo "error: DBG must be 'ddd' or 'gdb', got '$DBG'" >&2; exit 1
        ;;
esac

# Whichever pane's program exits first tears the whole session down, so quitting
# the debugger or QEMU (or closing either) drops you straight back to your shell.
TEARDOWN="tmux kill-session -t $TMUX_SESS"

# Pane 1 (top): build ISO + HDD, then run QEMU halted with the gdbstub open.
QEMU_CMD="make _debug-qemu; $TEARDOWN"

# Pane 2 (bottom): wait for QEMU to start listening, then attach the debugger.
DBG_CMD=$(cat <<EOF
echo "waiting for QEMU gdbstub on :$GDB_PORT ..."
for i in \$(seq 1 $GDB_WAIT); do
    if (exec 3<>/dev/tcp/127.0.0.1/$GDB_PORT) 2>/dev/null; then exec 3>&- 3<&-; break; fi
    sleep 1
done
$DBG_LAUNCH
$TEARDOWN
EOF
)

# Start fresh each time so stale panes never linger.
tmux kill-session -t "$TMUX_SESS" 2>/dev/null || true

tmux new-session  -d -s "$TMUX_SESS" -n debug "$QEMU_CMD"
tmux split-window -v -t "$TMUX_SESS:debug" "$DBG_CMD"
tmux set-option   -t "$TMUX_SESS" mouse on
tmux select-pane  -t "$TMUX_SESS:debug.{top}"

if [ -n "${TMUX:-}" ]; then
    exec tmux switch-client -t "$TMUX_SESS"
else
    exec tmux attach-session -t "$TMUX_SESS"
fi
