# Flycapture-Image-Processing

This project demonstrates a KWIVER process written in C++ that takes an image using a [Flycapture](https://www.ptgrey.com/flycapture-sdk) camera and pushes it to a pipeline. The example pipeline runs a [face detector](https://github.com/hdefazio/face_detection) on the image taken from the camera. 

# Organization #

File Name                             |  Description
--------------------------------------|------------------------------------------------------------------------------------------
[face_detection_flycap_test.pipe](https://github.com/hdefazio/Flycapture-Image-Processing/blob/master/face_detection_flycap_test.pipe) | Sprokit pipeline file that runs the face detector on an image read from the Flycapture camera
[CMakeLists.txt](https://github.com/hdefazio/Flycapture-Image-Processing/blob/master/CMakeLists.txt) | Flycapture input process source and header files added
[flycapture_input_process.cxx](https://github.com/hdefazio/Flycapture-Image-Processing/blob/master/flycapture_input_process.cxx) | Flycapture process written in C++
[flycapture_input_process.h](https://github.com/hdefazio/Flycapture-Image-Processing/blob/master/flycapture_input_process.h) | Flycapture input header file
[register_processes.cxx](https://github.com/hdefazio/Flycapture-Image-Processing/blob/master/register_processes.cxx) | Flycapture input added to register the process
[output](https://github.com/hdefazio/Flycapture-Image-Processing/blob/master/output) | Output directory where the annotated images are stored


# Building #
  In a bash terminal in the kwiver source directory::
  
    cmake -S . -B ../build
  
  In the kwiver build directory::
  
    make -j7
  
# Running #

## C++ ##

  To use an image, in <path/to/kwiver/build>/bin::
  
    ./pipeline_runner -p ../examples/pipelines/face_detection_flycap_test.pipe
  
  After it has finished running, the annotated image will be in ../examples/pipelines/output

