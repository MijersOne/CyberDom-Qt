#!/usr/bin/env bash
set -e

sudo apt-get update
sudo apt-get install -y \
    qt6-base-dev qt6-multimedia-dev qt6-tools-dev qmake6

echo "Qt packages installed. Run 'cmake ..' inside build directory again or use qmake6."
echo "To build tests, execute: qmake6 tests/tests.pro && make -C tests"

