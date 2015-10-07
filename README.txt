To compile this OTB module, OpenCV is required.
Remember to set OTB_USE_OPENCV to ON.

Even though it is in the dependency list in otb-module.cmake, it will not be
automatically resolved. This is because OpenCV is in an OTB_USE_* and the value
of those CMake variables overwrites their modules activation states.

