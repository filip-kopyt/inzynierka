#!/bin/env python

import multiprocessing
import pathlib
import sys

import cv2 as cv
import numpy as np
import tqdm


def raw2png(src_dst: tuple[str, str]):
    src, dest = src_dst
    raw = np.fromfile(src, dtype=np.uint8)
    img = raw.reshape((1300, 1600 * 2))
    cv.imwrite(dest, img, [cv.IMWRITE_PNG_COMPRESSION, 9])


def main():
    src = pathlib.Path(sys.argv[1])
    dest = pathlib.Path(sys.argv[2])
    if src.is_dir() and dest.is_dir():
        src_files = [f for f in src.iterdir() if f.name.endswith(".raw")]
        dest_files = [dest / f.name.replace(".raw", ".png") for f in src_files]
        with multiprocessing.Pool(6) as p:
            _ = list(
                tqdm.tqdm(
                    p.imap(
                        raw2png,
                        [
                            (str(src), str(dst))
                            for (src, dst) in zip(src_files, dest_files)
                        ],
                    ),
                    total=len(src_files),
                )
            )
    elif src.is_file():
        dest = dest / src.name.replace(".raw", ".png") if dest.is_dir() else dest
        raw2png((str(src), str(dest)))
    else:
        print("bad args")


if __name__ == "__main__":
    main()
