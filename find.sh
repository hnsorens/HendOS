#!/bin/bash

# Check for a hex pattern argument
if [ -z "$1" ]; then
    echo "Usage: $0 '37 00 00 00 e8 91 c1 00'"
    echo "Example: $0 '48 89 e5'"
    exit 1
fi

PATTERN=$(echo "$1" | tr '[:lower:]' '[:upper:]') # Normalize to uppercase
TMPFILE=$(mktemp)

# Recursively find all .o files
find . -type f -name "*.o" | while read -r objfile; do
    # Dump hex into temporary file
    hexdump -v -e '/1 "%02X "' "$objfile" > "$TMPFILE"

    # Check if pattern is in hex dump
    if grep -q "$PATTERN" "$TMPFILE"; then
        echo "Pattern found in: $objfile"
    fi
done

rm "$TMPFILE"
