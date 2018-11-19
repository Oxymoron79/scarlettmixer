# Scarlett Mixer
Scarlett Mixer is a GTK+3 mixer application for the Focusrite Scarlett USB audio
interfaces using the ALSA mixer interface.

## Supported Interfaces
The application is developed and tested with a Scarlett 6i6 interface,
but should also work with other Scarlett interfaces.

## Build Instructions
The toolchain to build the application from source depends on these tools:
- autoconf >= 2.59
- automake >= 1.11
- pkg-config >= 0.26
- gcc >= 4.4

To enable the build the developer documentation these tools should be installed:
- doxygen >= 1.8
- graphviz >= 2.40

The application depends on these libraries:
- alsa-lib >= 1.0.0
- Gtk+ >= 3.16
- JSON-Glib >= 1.0

Make sure the development headers of the libraries are also installed.

The application is built with these commands:
```
autoreconf -i
./configure
make
```
Install the application:
```
sudo make install
```

## How to report bugs
Bugs should be reported to the GitHub issues tracking system
(http://www.github.com/Oxymoron79/scarlettmixer/issues). 
You will need to create an account for yourself.
