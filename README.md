# CyberDom-Qt

CyberDom-Qt is a Qt-based graphical interface for running CyberDom training scripts. The application guides the user through tasks defined in a script while handling jobs, punishments and rewards.

## Prerequisites

Install the following Qt packages on Debian/Ubuntu:

```bash
sudo apt-get install qt6-base-dev qt6-multimedia-dev qt6-tools-dev qmake6
```

A helper script `setup-debian.sh` is provided to automate this. After installing the packages, rerun `cmake ..` in the `build` directory if you configured the project before installing dependencies.

## Build

Ensure a Qt development environment is available and CMake 3.14 or newer is installed:

```bash
mkdir build && cd build
cmake ..
make
```

Alternatively, build directly with `qmake6`:

```bash
qmake6 CyberDom.pro
make
```

### Tests

Unit tests use the Qt Test framework and rely on `qmake6`. Make sure the
`qmake6` package is installed before building:

```bash
qmake6 tests/tests.pro
make -C tests
```

Execute the tests in a headless environment:

```bash
QT_QPA_PLATFORM=offscreen ./tests/runtests
```

Alternatively you can run `make -C tests test` to build and execute the tests in one step.

You can also open `CyberDom.pro` in Qt Creator and build directly from the IDE.

## Run

After compiling, execute the generated binary:

```bash
./CyberDom
```

## Features

- Dynamic **Report** submenu under Communication. Items are populated on startup.
- The submenu includes **Add Clothing** which opens a popup to create clothing items.

### Birthday sections

Define birthdays in the training script using `[birthday-*]` sections. The part after
`birthday-` becomes the default title unless a `Title` key is provided. Each section
requires one or more `Date` lines in `YYYY-MM-DD` format.

Example:

```ini
[birthday-jane]
Title=Jane Doe
Date=2025-03-14
```

Birthday entries show up in the calendar view as events of type **Birthday**.

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
