import pathlib
import sys

import cv2 as cv
import numpy as np

PATTERN_SHAPE = np.zeros((10 * 7, 3), np.float32)
PATTERN_SHAPE[:, :2] = np.mgrid[0:10, 0:7].T.reshape(-1, 2)

SUBPIX_CRITERIA = (cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER, 30, 0.001)

IMAGE_SIZE = (1600, 1300)


def calibrate_mono(
    images: list[pathlib.Path],
) -> tuple[cv.typing.MatLike, cv.typing.MatLike, list[np.ndarray], list[np.ndarray]]:
    objpoints: list[cv.typing.MatLike] = []
    imgpoints: list[cv.typing.MatLike] = []

    for fname in images:
        img = cv.imread(fname)
        assert img is not None
        img = cv.cvtColor(img, cv.COLOR_RGB2GRAY)

        ret, corners = cv.findChessboardCorners(img, (10, 7), None)
        assert ret

        objpoints.append(PATTERN_SHAPE)
        corners2 = cv.cornerSubPix(img, corners, (11, 11), (-1, -1), SUBPIX_CRITERIA)
        imgpoints.append(corners2)

    err, mtx, dist, _, _ = cv.calibrateCamera(
        objpoints, imgpoints, IMAGE_SIZE, None, None
    )
    print(f"Reprojection error: {err}")

    return mtx, dist, imgpoints, objpoints


def main():
    path = pathlib.Path(sys.argv[1])

    paths_l = list((path / "left").iterdir())
    paths_l.sort()
    mtxR, distR, imgpR, objp = calibrate_mono(paths_l)

    paths_r = list((path / "right").iterdir())
    paths_r.sort()
    mtxL, distL, imgpL, _ = calibrate_mono(paths_r)

    err, mtxL, distL, mtxR, distR, R, T, _, _ = cv.stereoCalibrate(
        objp, imgpL, imgpR, mtxL, distL, mtxR, distR, IMAGE_SIZE
    )
    print(f"Stereo reprojection err: {err}")

    file = cv.FileStorage("calibration.ext", cv.FILE_STORAGE_WRITE)
    file.write("K1", mtxL)
    file.write("D1", distL)
    file.write("K2", mtxR)
    file.write("D2", distR)
    file.write("R", R)
    file.write("T", T)
    file.release()


if __name__ == "__main__":
    main()
