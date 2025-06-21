# CyberDom-Qt

CyberDom-Qt is a Qt-based graphical interface for running CyberDom training scripts. The application guides the user through tasks defined in a script while handling jobs, punishments and rewards.

## Build

Ensure a Qt development environment is available and CMake 3.14 or newer is installed:

```bash
mkdir build && cd build
cmake ..
make
```

You can also open `CyberDom.pro` in Qt Creator and build directly from the IDE.

## Run

After compiling, execute the generated binary:

```bash
./CyberDom
```

## Features

- Dynamic **Report** submenu under Communication. Items are populated on startup.
- The submenu includes **Add Clothing** which opens a popup to create clothing items.

## Debugging

Runtime messages are written to `debug_output.log` in the application directory. Use this log when diagnosing crashes or other issues.
