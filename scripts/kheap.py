"""kheap - track lizard's buddy / slab / kmalloc allocators from gdb (or ddd).

Loaded automatically by script.gdb (`source scripts/kheap.py`); to load by hand
from a gdb prompt run the same command from the repo root.

Commands:
    kheap                     print the tracking summary table
    kheap on [layer...]       install tracking breakpoints (default: all layers)
    kheap off                 remove them
    kheap list [layer]        list outstanding allocations (buddy | slab | kmalloc)
    kheap trace on | off      echo every alloc / free as it happens
    kheap reset               zero the counters and forget the live set

Point-in-time structure dumps (no tracking needed, safe to run any time):
    kheap caches              one row per slab cache (walks kmalloc_cache_list)
    kheap cache <name|addr>   one cache in detail + the `graph display` expr for it
    kheap buddy               free-block count per order

Every tracked allocator call halts the VM while gdb runs a little Python, so a
full boot with tracking on is slow. Boot with it off, stop where you care
(the script breaks at kmain), then `kheap on`. Narrow the overhead by naming
layers, e.g. `kheap on kmalloc` when the buddy/slab traffic is just noise.

A single kmalloc() usually shows up in all three layers: kmalloc -> a slab
cache -> buddy pages, each recorded against its own layer.
"""

import gdb

PAGE_SIZE = 4096
_LAYER_NAMES = ("buddy", "slab", "kmalloc")


class Layer:
    def __init__(self, name):
        self.name = name
        self.live = {}      # addr -> [size, tag, backtrace]
        self.freed = set()  # addrs seen freed since last reset (double-free check)
        self.n_alloc = 0
        self.n_free = 0
        self.n_double = 0
        self.n_unknown = 0
        self.bytes = 0      # bytes currently live

    def reset(self):
        self.__init__(self.name)


LAYERS = {n: Layer(n) for n in _LAYER_NAMES}
TRACE = False
_bps = []


def _out(msg):
    gdb.write(msg + "\n")


def _trace(msg):
    if TRACE:
        gdb.write("[kheap] " + msg + "\n")


def _bt(limit=4):
    """Caller chain above the allocator entry frame, as 'a <- b <- c'."""
    names = []
    try:
        f = gdb.newest_frame()
        f = f.older() if f else None
        while f is not None and len(names) < limit:
            names.append(f.name() or "??")
            f = f.older()
    except gdb.error:
        pass
    return " <- ".join(names) if names else "?"


# --- return-value capture --------------------------------------------------

class _AllocRet(gdb.FinishBreakpoint):
    """Fires when an alloc function returns; records the pointer it handed back."""

    def __init__(self, layer, size, tag, bt):
        # ValueError here == called from a gdb `call`/`print f()` dummy frame;
        # _Entry.stop swallows it and that one allocation just goes untracked.
        super().__init__(gdb.newest_frame(), internal=True)
        self.layer, self.size, self.tag, self.bt = layer, size, tag, bt

    def stop(self):
        try:
            addr = int(self.return_value)
        except (gdb.error, TypeError, ValueError):
            addr = 0
        if addr:
            L = self.layer
            L.live[addr] = [self.size, self.tag, self.bt]
            L.freed.discard(addr)
            L.n_alloc += 1
            L.bytes += self.size
            _trace("%-7s alloc 0x%x  %6d B  %-18s [%s]"
                   % (L.name, addr, self.size, self.tag, self.bt))
        return False

    def out_of_scope(self):
        _trace("%s alloc: frame left without returning" % self.layer.name)


class _KreallocRet(gdb.FinishBreakpoint):
    def __init__(self, oldp, newsize, bt):
        super().__init__(gdb.newest_frame(), internal=True)
        self.oldp, self.newsize, self.bt = oldp, newsize, bt

    def stop(self):
        L = LAYERS["kmalloc"]
        try:
            newp = int(self.return_value)
        except (gdb.error, TypeError, ValueError):
            newp = 0
        if newp == 0:
            return False  # realloc failed, old block still live
        if self.oldp in L.live:
            L.bytes -= L.live.pop(self.oldp)[0]
            L.freed.add(self.oldp)
            L.n_free += 1
        L.live[newp] = [self.newsize, "krealloc", self.bt]
        L.freed.discard(newp)
        L.bytes += self.newsize
        L.n_alloc += 1
        _trace("kmalloc realloc 0x%x -> 0x%x  %d B" % (self.oldp, newp, self.newsize))
        return False


# --- entry breakpoints ---------------------------------------------------------

class _Entry(gdb.Breakpoint):
    def __init__(self, spec):
        super().__init__(spec, internal=False)
        self.silent = True

    def stop(self):
        try:
            self.handle(gdb.newest_frame())
        except (gdb.error, ValueError) as e:
            _trace("%s: %s" % (self.location, e))
        return False


class _BuddyAlloc(_Entry):
    def handle(self, fr):
        order = int(fr.read_var("order"))
        _AllocRet(LAYERS["buddy"], (1 << order) * PAGE_SIZE, "order %d" % order, _bt())


class _SlabAlloc(_Entry):
    def handle(self, fr):
        cache = fr.read_var("cache")
        try:
            name = cache["name"].string()
        except gdb.error:
            name = "?"
        _AllocRet(LAYERS["slab"], int(cache["object_size"]), name, _bt())


class _KmallocAlloc(_Entry):
    def handle(self, fr):
        _AllocRet(LAYERS["kmalloc"], int(fr.read_var("size")), "kmalloc", _bt())


class _KreallocEntry(_Entry):
    def handle(self, fr):
        _KreallocRet(int(fr.read_var("ptr")), int(fr.read_var("new_size")), _bt())


def _do_free(layer, addr):
    if addr == 0:
        return
    L = layer
    if addr in L.live:
        size, tag, _ = L.live.pop(addr)
        L.freed.add(addr)
        L.n_free += 1
        L.bytes -= size
        _trace("%-7s free  0x%x  %6d B  (%s)" % (L.name, addr, size, tag))
    elif addr in L.freed:
        L.n_double += 1
        _trace("%-7s DOUBLE FREE 0x%x" % (L.name, addr))
    else:
        L.n_unknown += 1
        _trace("%-7s free  0x%x  <untracked>" % (L.name, addr))


class _BuddyFree(_Entry):
    def handle(self, fr):
        _do_free(LAYERS["buddy"], int(fr.read_var("vaddr")))


class _SlabFree(_Entry):
    def handle(self, fr):
        _do_free(LAYERS["slab"], int(fr.read_var("obj")))


class _KfreeFree(_Entry):
    def handle(self, fr):
        _do_free(LAYERS["kmalloc"], int(fr.read_var("ptr")))


_SPECS = (
    ("buddy", _BuddyAlloc, "buddy_alloc"),
    ("buddy", _BuddyFree, "buddy_free"),
    ("slab", _SlabAlloc, "kmemcache_alloc"),
    ("slab", _SlabFree, "kmemcache_free"),
    ("kmalloc", _KmallocAlloc, "kmalloc"),
    ("kmalloc", _KfreeFree, "kfree"),
    ("kmalloc", _KreallocEntry, "krealloc"),  # optional: gc'd out when unused
)


def _install(layers):
    if _bps:
        _out("kheap: already tracking - `kheap off` first")
        return
    for layer, cls, sym in _SPECS:
        if layer not in layers:
            continue
        try:
            _bps.append(cls(sym))
        except Exception as e:
            _out("kheap: skipping %s (%s)" % (sym, e))
    _out("kheap: tracking %s (%d entry points) - `kheap` for the table"
         % ("/".join(sorted(layers)), len(_bps)))


def _remove():
    for bp in _bps:
        try:
            bp.delete()
        except gdb.error:
            pass
    _bps.clear()
    _out("kheap: tracking off")


# --- structure dumps (independent of tracking) ------------------------------

def _list_len(head):
    """Number of entries in an intrusive list, given the gdb.Value list_head."""
    try:
        stop = int(head.address)
        lh_p = gdb.lookup_type("struct list_head").pointer()
        node = int(head["next"])
        n = 0
        while node and node != stop and n < 1 << 20:
            node = int(gdb.Value(node).cast(lh_p)["next"])
            n += 1
        return n
    except gdb.error:
        return -1


def _iter_caches():
    """Yield every struct kmem_cache* we can reach from the kmalloc layer."""
    seen = set()
    try:
        nc = gdb.parse_and_eval("kmalloc_node_cache")
        if int(nc) and int(nc) not in seen:
            seen.add(int(nc))
            yield nc
    except gdb.error:
        pass
    try:
        head = gdb.parse_and_eval("kmalloc_cache_list")
        stop = int(head.address)
        cn_p = gdb.lookup_type("struct cache_node").pointer()
        node = int(head["next"])
        guard = 0
        while node and node != stop and guard < 1 << 20:
            cn = gdb.Value(node).cast(cn_p)
            cache = cn["cache"]
            if int(cache) and int(cache) not in seen:
                seen.add(int(cache))
                yield cache
            node = int(cn["list"]["next"])
            guard += 1
    except gdb.error as e:
        _out("kheap: cannot walk kmalloc_cache_list (%s)" % e)


def _caches():
    hdr = "%-16s %8s %8s %7s %8s %4s %5s %5s %5s" % (
        "cache", "obj", "real", "in_use", "per-slab", "ord", "full", "part", "free")
    _out(hdr)
    _out("-" * len(hdr))
    found = False
    for c in _iter_caches():
        found = True
        try:
            name = c["name"].string()
        except gdb.error:
            name = "?"
        try:
            _out("%-16s %8d %8d %7d %8d %4d %5d %5d %5d" % (
                name[:16], int(c["object_size"]), int(c["real_object_size"]),
                int(c["in_use"]), int(c["objects_per_slab"]), int(c["order"]),
                _list_len(c["slabs_full"]), _list_len(c["slabs_partial"]),
                _list_len(c["slabs_free"])))
        except gdb.error as e:
            _out("%-16s  <unreadable: %s>" % (name[:16], e))
    if not found:
        _out("(no caches - kmalloc_cache_list empty or kmalloc not yet initialised)")


def _cache(key):
    for c in _iter_caches():
        try:
            name = c["name"].string()
        except gdb.error:
            name = ""
        if key in (name, hex(int(c)), str(int(c))):
            _out("struct kmem_cache @ 0x%x  '%s'" % (int(c), name))
            for f in ("object_size", "real_object_size", "align", "size", "in_use",
                      "free_slab_count", "objects_per_slab", "order", "flags"):
                try:
                    _out("  %-18s %d" % (f, int(c[f])))
                except gdb.error:
                    pass
            for lst in ("slabs_full", "slabs_partial", "slabs_free"):
                _out("  %-18s %d slab(s)" % (lst, _list_len(c[lst])))
            _out("  graph display *(struct kmem_cache *)0x%x" % int(c))
            return
    _out("kheap: no cache matching %r - try `kheap caches`" % key)


def _buddy():
    try:
        b = gdb.parse_and_eval("buddy")
        _out("buddy: %d pages tracked" % int(b["page_count"]))
    except gdb.error as e:
        _out("kheap: %s" % e)
        return
    _out("  %-5s %12s %12s" % ("order", "free-blocks", "free-bytes"))
    lo, hi = b["free_areas"].type.range()
    total = 0
    for o in range(lo, hi + 1):
        try:
            fc = int(b["free_areas"][o]["free_count"])
        except gdb.error:
            break
        if fc:
            nbytes = fc * (1 << o) * PAGE_SIZE
            total += nbytes
            _out("  %-5d %12d %12d" % (o, fc, nbytes))
    _out("  %-5s %12s %12d total free" % ("", "", total))
    _out("  graph display buddy")


# --- tracking reporting ----------------------------------------------------

def _summary():
    hdr = "%-8s %8s %8s %8s %13s %8s %8s" % (
        "layer", "alloc", "free", "live", "bytes-live", "dbl-fr", "unk-fr")
    _out(hdr)
    _out("-" * len(hdr))
    for name in _LAYER_NAMES:
        L = LAYERS[name]
        _out("%-8s %8d %8d %8d %13d %8d %8d" % (
            name, L.n_alloc, L.n_free, len(L.live), L.bytes, L.n_double, L.n_unknown))
    if not _bps:
        _out("(tracking is OFF - run `kheap on`)")


def _list(which):
    names = _LAYER_NAMES if which is None else (which,)
    for name in names:
        L = LAYERS.get(name)
        if L is None:
            _out("kheap: unknown layer %r (buddy | slab | kmalloc)" % name)
            continue
        _out("== %s: %d live, %d bytes ==" % (name, len(L.live), L.bytes))
        for addr in sorted(L.live):
            size, tag, bt = L.live[addr]
            _out("  0x%016x  %8d B  %-18s %s" % (addr, size, tag, bt))


class KHeap(gdb.Command):
    __doc__ = __doc__

    def __init__(self):
        super().__init__("kheap", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        global TRACE
        argv = gdb.string_to_argv(arg)
        if not argv:
            _summary()
            return
        cmd = argv[0]
        if cmd == "on":
            want = argv[1:] or list(_LAYER_NAMES)
            bad = [x for x in want if x not in _LAYER_NAMES]
            if bad:
                _out("kheap: unknown layer(s): %s (buddy | slab | kmalloc)" % " ".join(bad))
            else:
                _install(set(want))
        elif cmd == "off":
            _remove()
        elif cmd == "reset":
            for L in LAYERS.values():
                L.reset()
            _out("kheap: counters cleared")
        elif cmd == "list":
            _list(argv[1] if len(argv) > 1 else None)
        elif cmd == "caches":
            _caches()
        elif cmd == "cache":
            if len(argv) > 1:
                _cache(argv[1])
            else:
                _out("usage: kheap cache <name|addr>")
        elif cmd == "buddy":
            _buddy()
        elif cmd == "trace":
            TRACE = len(argv) > 1 and argv[1] == "on"
            _out("kheap: trace %s" % ("on" if TRACE else "off"))
        else:
            _out("usage: kheap [on|off | list [layer] | caches | cache <k> | "
                 "buddy | trace on|off | reset]")


KHeap()
_out("kheap: loaded - `kheap on` to start tracking buddy / slab / kmalloc")
