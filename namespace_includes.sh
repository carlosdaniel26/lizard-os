#!/bin/bash
# Find all headers in lizard and nolibc
headers=$(find lizard nolibc -name "*.h")

for h in $headers; do
    basename=$(basename "$h")
    # Identify which namespace it belongs to
    if [[ $h == lizard/* ]]; then
        namespace="lizard"
    elif [[ $h == nolibc/* ]]; then
        namespace="nolibc"
    else
        continue
    fi
    
    # Replace #include <header.h> with #include <namespace/header.h>
    # Only replace if not already prefixed
    find . -type f \( -name "*.c" -o -name "*.h" \) -exec sed -i "s|#include <$basename>|#include <$namespace/$basename>|g" {} +
done
