#!/bin/bash

cd ~/Repos/fcs && \
    sudo stop fcs-server;
make clean && \
    make CPPFLAGS='-DACTIVE_BOARD -DSI57x_FOUT=113040445' && \
    sudo make install && \
    sudo start fcs-server
