#!/bin/bash

if [[ $# -ne 1 ]]; then
    echo "usage:" $(basename "$0") "<path to folders with frames>"
    exit 0
fi

for dir in $(find $1 -maxdepth 1 -mindepth 1 -type d)
do
    echo "compressing $dir into $dir.7z"
    7zz a "$dir.7z" "$dir" && 7zz rn "$dir.7z" "$dir" $(basename "$dir")
    if [[ -f "$dir.7z" ]]; then
    	echo "deleting $dir"
    	rm -rf "$dir"
    fi
done
