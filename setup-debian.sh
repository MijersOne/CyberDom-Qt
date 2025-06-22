#!/usr/bin/env bash
set -e

sudo apt-get update
sudo apt-get install -y \
    qtbase5-dev qt5-qmake qtchooser libqt5multimedia5-dev

echo "Qt packages installed. Run 'cmake ..' inside build directory again."

