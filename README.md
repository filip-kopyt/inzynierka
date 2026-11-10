# Engineering thesis repository

Collection of tools and script used in my engineering thesis.

## Collection Pipeline

Main C++ app tasked with synchronizing stereo camera, collecting and writing frames onto the disk.

TODO: frames rectifying, disparity calculation, drone detection

## Tools

Collection of tools used for offline processing/one time generation. List include:
 - `compress_frames.sh <folder>` will compress all subfolders of `folder` into `.7z` archives
 - `get_camera_intrisics.py <folder>` will generate `calibration.ext` file containing stereo camera parameters used in rectification based on images in `<folder>/left/` and `<folder>/right`
 - `raw2png.py <input> <output>` will convert raw camera frame in `<input>` to png image in `<output>`. To convert multiple frames, supply directories as both `<input>` and `<output>`.
 - `repack_matlab_mat.py <*.mat file>` will convert `*.mat` file containing rectification parameters into `calibration.ext` file used by this repository