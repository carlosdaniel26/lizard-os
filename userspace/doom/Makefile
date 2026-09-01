## doomgeneric port, delivered as a patch against upstream.
##
## This clones ozkl/doomgeneric at a pinned commit into ./doomgeneric/
## (gitignored) and applies doomgeneric.patch, which adds the lizard platform
## layer (doomgeneric/dg_lizard.c) and its build (doomgeneric/Makefile.lizard).
## The first build needs network access; after that it is offline.
##
## Output: ./doom  (an ET_EXEC ELF, staged into the disk image as /DOOM).

DG_URL    := https://github.com/ozkl/doomgeneric
DG_COMMIT := dcb7a8dbc7a16ce3dda29382ac9aae9d77d21284
DG_DIR    := doomgeneric
PATCH     := doomgeneric.patch

.PHONY: all clean

all: doom

## Re-clone whenever the patch changes so a stale tree can't linger.
$(DG_DIR)/.patched: $(PATCH)
	@command -v git >/dev/null || { echo "ERROR: git is required to fetch doomgeneric"; exit 1; }
	rm -rf $(DG_DIR)
	git clone --quiet $(DG_URL) $(DG_DIR)
	git -C $(DG_DIR) -c advice.detachedHead=false checkout --quiet $(DG_COMMIT)
	git -C $(DG_DIR) apply --whitespace=nowarn ../$(PATCH)
	touch $@

doom: $(DG_DIR)/.patched
	$(MAKE) -C $(DG_DIR)/doomgeneric -f Makefile.lizard

clean:
	rm -rf $(DG_DIR) doom
