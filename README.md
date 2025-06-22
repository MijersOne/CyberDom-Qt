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


### Report menu troubleshooting

The Report menu is generated at runtime from the loaded script. If the application crashes when the window first appears, open `debug_output.log` while launching `CyberDom` from a terminal:

```bash
./CyberDom &
tail -f debug_output.log
```

Each menu entry will be listed with a `[ReportMenu]` prefix as it is created. A crash after a specific entry often indicates a problem in the script definition for that report.

For additional help, review the example scripts included with the project and open an issue if problems persist.
