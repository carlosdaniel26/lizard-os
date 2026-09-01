file build/bin/kernel
target remote :1234
source scripts/kheap.py
break kmain
continue