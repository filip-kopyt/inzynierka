# import cv2 as cv
# import numpy as np
# import pathlib

# PATTERN_SHAPE = np.zeros((10 * 7, 3), np.float32)
# PATTERN_SHAPE[:, :2] = np.mgrid[0:10, 0:7].T.reshape(-1,2)

# SUBPIX_CRITERIA = (cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER, 30, 0.001)

# IMAGE_SIZE = (1600, 1300)

# def calibrate_mono(images: list[pathlib.Path]) -> tuple[cv.typing.MatLike, cv.typing.MatLike, list[np.ndarray], list[np.ndarray]]:
#     objpoints: list[cv.typing.MatLike] = []
#     imgpoints: list[cv.typing.MatLike] = []
    
#     for fname in images:
#         img = cv.imread(fname)
#         assert img is not None
#         img = cv.cvtColor(img, cv.COLOR_RGB2GRAY)

#         ret, corners = cv.findChessboardCorners(img, (10, 7), None)
#         assert ret

#         objpoints.append(PATTERN_SHAPE)
#         corners2 = cv.cornerSubPix(img, corners, (11, 11), (-1, -1), SUBPIX_CRITERIA)
#         imgpoints.append(corners2)

#     err, mtx, dist, rvecs, tvecs = cv.calibrateCamera(objpoints, imgpoints, IMAGE_SIZE, None, None)
#     print(f"Reprojection error: {err}")
    
#     return mtx, dist, imgpoints, objpoints

# paths = [path for path in pathlib.Path("..").iterdir() if path.name.endswith("L.png")]
# paths.sort()
# mtxR, distR, imgpR, objp = calibrate_mono(paths)

# paths = [path for path in pathlib.Path("..").iterdir() if path.name.endswith("R.png")]
# paths.sort()
# mtxL, distL, imgpL, _ = calibrate_mono(paths)

# err, mtxL, distL, mtxR, distR, R, T, E, F = cv.stereoCalibrate(objp, imgpL, imgpR, mtxL, distL, mtxR, distR, IMAGE_SIZE)
# print(f"Stereo reprojection err: {err}")

# R1, R2, P1, P2, Q, roi1, roi2 = cv.stereoRectify(mtxL, distL, mtxR, distR, IMAGE_SIZE, R, T)

# map1L, map2L = cv.initUndistortRectifyMap(mtxL, distL, R1, P1, IMAGE_SIZE, cv.CV_32FC1)
# map1R, map2R = cv.initUndistortRectifyMap(mtxR, distR, R2, P2, IMAGE_SIZE, cv.CV_32FC1)

# file = cv.FileStorage("calibration.ext", cv.FILE_STORAGE_WRITE)
# file.write("mtx_left", mtxL)
# file.write("dist_left", distL)
# file.write("P_left", P1)
# file.write("R_left", R1)
# file.write("mtx_right", mtxR)
# file.write("dist_right", distR)
# file.write("P_right", P2)
# file.write("R_right", R2)
# file.write("Q", Q)
# file.release()

# imgL = cv.imread("../frame0_R.png")
# imgR = cv.imread("../frame0_L.png")

# assert imgL is not None
# assert imgR is not None

# imgL = cv.cvtColor(imgL, cv.COLOR_RGB2GRAY)
# imgR = cv.cvtColor(imgR, cv.COLOR_RGB2GRAY)

# imgL = cv.remap(imgL, map1L, map2L, cv.INTER_LINEAR)
# imgR = cv.remap(imgR, map1R, map2R, cv.INTER_LINEAR)

#x, y, w, h = roi1
#imgL = imgL[y:y+h, x:x+w]

#x, y, w, h = roi2
#imgR = imgR[y:y+h, x:x+w]

#imgR = cv.resize(imgR, imgL.shape[::-1])

# merged = cv.hconcat([imgL, imgR])
# merged = cv.cvtColor(merged, cv.COLOR_GRAY2BGR)

# for i in range (50, 1000, 50):
#     merged = cv.line(merged, (0, i), (3150, i), (0, 0, 255), 2)

# cv.imwrite("test.png", merged)

# matcher = cv.StereoSGBM.create(
#     minDisparity=0,
#     numDisparities=256,
#     blockSize=5,
#     P1=8 * 1 * 5 ** 2,
#     P2=32 * 1 * 5 ** 2,
#     disp12MaxDiff=3,
#     uniquenessRatio=15,
#     speckleWindowSize=100,
#     speckleRange=2,
#     mode=cv.STEREO_SGBM_MODE_SGBM_3WAY,
# )

# right_matcher = cv.ximgproc.createRightMatcher(matcher)
# dispL = matcher.compute(imgL, imgR)
# dispR = right_matcher.compute(imgR, imgL)

# wls_filter = cv.ximgproc.createDisparityWLSFilter(matcher)
# wls_filter.setLambda(25000)
# wls_filter.setSigmaColor(0.6)
# filtered_disp = wls_filter.filter(dispL, imgL, disparity_map_right=dispR)

# disp = filtered_disp.astype(np.float32) / 16.0
# disp_vis = cv.normalize(disp, None, alpha=0, beta=255,
#                         norm_type=cv.NORM_MINMAX, dtype=cv.CV_8U)

# cv.imwrite("test_disp.png", disp_vis)

import cv2 as cv
import numpy as np
from scipy.io import loadmat

data = loadmat("stereo_calibration.mat")

K1 = data["K1"].astype(np.float64)
D1 = data["D1"].reshape(-1, 1).astype(np.float64)

K2 = data["K2"].astype(np.float64)
D2 = data["D2"].reshape(-1, 1).astype(np.float64)

R = data["R"].astype(np.float64)
T = data["T"].reshape(3, 1).astype(np.float64)

image_size = (1300, 1600)

# MATLAB stores image size as [rows, columns].
# OpenCV expects (width, height) = (columns, rows).
height, width = image_size
image_size_cv = (width, height)

print("Camera 1 matrix:\n", K1)
print("Camera 1 distortion:\n", D1.ravel())
print("Camera 2 matrix:\n", K2)
print("Camera 2 distortion:\n", D2.ravel())
print("Rotation:\n", R)
print("Translation:\n", T.ravel())

# Compute stereo rectification
R1, R2, P1, P2, Q, roi1, roi2 = cv.stereoRectify(
    K1, D1,
    K2, D2,
    image_size_cv,
    R, T,
)

# Generate remapping tables
map1x, map1y = cv.initUndistortRectifyMap(
    K1, D1, R1, P1,
    image_size_cv, cv.CV_32FC1
)

map2x, map2y = cv.initUndistortRectifyMap(
    K2, D2, R2, P2,
    image_size_cv, cv.CV_32FC1
)

imgL = cv.imread("../left/frame0.png")
imgR = cv.imread("../right/frame0.png")

assert imgL is not None
assert imgR is not None

imgL = cv.cvtColor(imgL, cv.COLOR_RGB2GRAY)
imgR = cv.cvtColor(imgR, cv.COLOR_RGB2GRAY)

# Rectify images
imgL = cv.remap(
    imgL, map1x, map1y, cv.INTER_LINEAR
)

imgR = cv.remap(
    imgR, map2x, map2y, cv.INTER_LINEAR
)

#imgL = cv.resize(imgL, None, fx=0.5, fy=0.5, interpolation=cv.INTER_LINEAR)
#imgR = cv.resize(imgR, None, fx=0.5, fy=0.5, interpolation=cv.INTER_LINEAR)

matcher = cv.StereoSGBM.create(
    minDisparity=0,
    numDisparities=256,
    blockSize=7,
    P1=8 * 7 ** 2,
    P2=32 * 7 ** 2,
    disp12MaxDiff=3,
    uniquenessRatio=20,
    speckleWindowSize=100,
    speckleRange=2,
    mode=cv.STEREO_SGBM_MODE_HH4,
)

#matcher = cv.StereoBM.create(256, 5)

right_matcher = cv.ximgproc.createRightMatcher(matcher)
dispL = matcher.compute(imgL, imgR)
dispR = right_matcher.compute(imgR, imgL)

wls_filter = cv.ximgproc.createDisparityWLSFilter(matcher)
wls_filter.setLambda(35000)
wls_filter.setSigmaColor(0.6)
#wls_filter.setDepthDiscontinuityRadius(3)
filtered_disp = wls_filter.filter(dispL, imgL, disparity_map_right=dispR)

disp = filtered_disp.astype(np.float32) / 16.0
dispL_vis = cv.normalize(dispL, None, alpha=0, beta=255,
                        norm_type=cv.NORM_MINMAX, dtype=cv.CV_8U)
disp_vis = cv.normalize(disp, None, alpha=0, beta=255,
                        norm_type=cv.NORM_MINMAX, dtype=cv.CV_8U)


merged = cv.hconcat([imgL, imgR])
cv.imwrite("test.png", merged)
cv.imwrite("test_disp_orig_filter.png", disp_vis)
cv.imwrite("test_disp_orig.png", dispL_vis)

file = cv.FileStorage("calibration.ext", cv.FILE_STORAGE_WRITE)
file.write("K1", K1)
file.write("D1", D1)
file.write("K2", K2)
file.write("D2", D2)
file.write("R", R)
file.write("T", T)
file.release()