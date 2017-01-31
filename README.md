Building on Linux
=================

* Install build tools: `sudo apt-get install cmake blender`.
* Install dependencies: `sudo apt-get install libvorbis-dev libflac-dev
  libopenal-dev libjpeg-dev libfreetype6-dev libudev-dev libgl-dev libglew-dev`.
* Configure build directory: `mkdir build && cd build && cmake ..`.
* Build: `make -j 4`.
