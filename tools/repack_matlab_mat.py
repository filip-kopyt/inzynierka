import sys

import cv2 as cv
import numpy as np
from scipy.io import loadmat


def main():
    data = loadmat(sys.argv[1])

    K1 = data["K1"].astype(np.float64)
    D1 = data["D1"].reshape(-1, 1).astype(np.float64)

    K2 = data["K2"].astype(np.float64)
    D2 = data["D2"].reshape(-1, 1).astype(np.float64)

    R = data["R"].astype(np.float64)
    T = data["T"].reshape(3, 1).astype(np.float64)

    file = cv.FileStorage("calibration.ext", cv.FILE_STORAGE_WRITE)
    file.write("K1", K1)
    file.write("D1", D1)
    file.write("K2", K2)
    file.write("D2", D2)
    file.write("R", R)
    file.write("T", T)
    file.release()


if __name__ == "__main__":
    main()
