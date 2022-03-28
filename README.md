# procwatch
Working example using netlink to monitor processes under linux

## Building
This source tree is configured to use CMake.

To generate a build configuration, perform the following steps:

1. mkdir build
2. cd build
3. cmake ..
4. make

Current documentation generated using doxygen has been included in docs/html
If python is installed, you can use it to spin up a temporary web server for
viewing the documentation:

python -m http.server 8080

This assumes that port 8080 is not already in use.  If it is, simply replace 8080
with a free TCP port.

If doxygen is in the path, the documentation can be generated using:

make docs
