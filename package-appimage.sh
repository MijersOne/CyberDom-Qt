#!/bin/bash

# --- Configuration ---
APP_NAME="CyberDom"
BUILD_DIR="build_appimage"
APP_DIR="AppDir"
EXECUTABLE_NAME="CyberDom"
DESKTOP_FILE="cyberdom.desktop"
ICON_FILE="cyberdom.png"
LINUXDEPLOYQT="./linuxdeployqt-continuous-x86_64.appimage"

# Check if appimagetool exists
if [ ! -f "$LINUXDEPLOYQT" ]; then
    echo "Error: $LINUXDEPLOYQT not found in current directory."
    echo "Please download it from: https://github.com/probonopd/go-appimage?tab=readme-ov-file"
    exit 1
fi

# Clean and Build (Release Mode)
echo "--- Building $APP_NAME ---"
rm -rf $BUILD_DIR
mkdir $BUILD_DIR
cd $BUILD_DIR

# Detect build system
if [ -f "../CMakeLists.txt" ]; then
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$Qt6_DIR ..
    make -j$(nproc)
elif [ -f "../$APP_NAME.pro" ]; then
    qmake ../$APP_NAME.pro CONFIG+=release
    make -j$(nproc)
else
    echo "Error: No CMakeLists.txt or .pro file found."
    exit 1
fi

cd ..

# Prepare AppDir Structure
echo "--- Preparing AppDir ---"
rm -rf $APP_DIR
mkdir -p $APP_DIR/usr/bin
mkdir -p $APP_DIR/usr/share/applications
mkdir -p $APP_DIR/usr/share/icons/hicolor/256x256/applications

# Copy Assets
echo "--- Copying Files ---"
# Copy Executable
cp $BUILD_DIR/$EXECUTABLE_NAME $APP_DIR/usr/bin/

# Copy Desktop File
cp $DESKTOP_FILE $APP_DIR/usr/share/applications/

# Copy Icon
if [ -f "$ICON_FILE" ]; then
    cp $ICON_FILE $APP_DIR/usr/share/icons/hicolor/256x256/apps/$ICON_FILE
else
    echo "Warning: Icon file $ICON_FILE not found. Using placeholder."
    touch $APP_DIR/usr/share/icons/hicolor/256x256/apps/cyberdom.png
fi

# Run linuxdeployqt
echo "--- Running linuxdeployqt ---"

# Set path to qmake if not in standard PATH
# export PATH=/path/to/qt/bin:$PATH

$LINUXDEPLOYQT $APP_DIR/usr/share/applications/$DESKTOP_FILE -appimage -unsupported-allow-new-glibc

echo "--- Done! ---"
echo "Your AppImage is located in the current directory."
