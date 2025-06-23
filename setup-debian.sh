#!/usr/bin/env bash
set -e

sudo apt-get update
sudo apt-get install -y \
    qt6-base-dev qt6-multimedia-dev qmake6

echo "Qt packages installed. Run 'cmake ..' inside build directory again or use qmake6."

