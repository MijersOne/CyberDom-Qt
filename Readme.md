# CyberDom

**CyberDom** is a robust, script-driven protocol enforcement and task management application built using C++ and the Qt Framework. It allows users to load custom `.ini` scripts that define dynamic rules, statuses, assignments, and behavior logic, creating a fully interactive and automated experience.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/D1D11DFKBC) 

## Overview

CyberDom functions as an automated controller that parses logic files to manage a user's daily routine, tasks, and compliance. It features a comprehensive event system that tracks merits, clothing requirements, and time-based events.

### Key Features
* **Script Engine:** Parses complex `.ini` scripts to define state machines, rules, and logic.
* **Assignment System:** Manages Jobs and Punishments with support for deadlines, reminders, and penalties.
* **Interactive Tasks:** Includes specialized interactive punishment modules like "Line Writing" (with anti-cheat mechanisms) and "Detention" (fullscreen lock with presence checks).
* **Multimedia Integration:** Supports webcam integration for "Point" and "Pose" tasks, as well as audio playback for alarms.
* **Economy System:** Tracks "Merits" with visual indicators for progress and penalty thresholds.
* **Reporting:** Generates detailed HTML daily activity reports tracking tasks completed, merits earned/lost, and permissions requested.
* **Status Management:** Handles hierarchical status changes (Primary and Sub-statuses) with history tracking.

## Technologies Used

* **Language:** C++17
* **Framework:** Qt 6 (Core, Gui, Widgets, Multimedia, Network)
* **Build Systems:** CMake / qmake

## Requirements

To build CyberDom from source, you will need the following installed on your system:

* **C++ Compiler:** GCC, Clang, or MSVC supporting C++17.
* **Qt 6 SDK:** Ensure the following modules are installed:
    * `Qt6::Core`
    * `Qt6::Gui`
    * `Qt6::Widgets`
    * `Qt6::Multimedia`
    * `Qt6::MultimediaWidgets`
    * `Qt6::Network`
* **Build Tool:** CMake (3.16 or higher) OR qmake.

## Build Instructions

You can build the application using either CMake or qmake.

### Option 1: Using CMake (Recommended)

1.  Clone the repository:
    ```bash
    git clone [https://github.com/yourusername/CyberDom.git](https://github.com/yourusername/CyberDom.git)
    cd CyberDom
    ```

2.  Create a build directory:
    ```bash
    mkdir build
    cd build
    ```

3.  Configure the project (ensure Qt is in your PATH or specify `CMAKE_PREFIX_PATH`):
    ```bash
    cmake ..
    ```

4.  Build the application:
    ```bash
    cmake --build .
    ```

### Option 2: Using qmake

1.  Clone the repository:
    ```bash
    git clone [https://github.com/yourusername/CyberDom.git](https://github.com/yourusername/CyberDom.git)
    cd CyberDom
    ```

2.  Run qmake:
    ```bash
    qmake CyberDom.pro
    ```

3.  Build the application:
    * **Linux/macOS:** `make`
    * **Windows (MinGW):** `mingw32-make`
    * **Windows (MSVC):** `nmake`

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/D1D11DFKBC)

## Usage

1.  Launch the `CyberDom` executable.
2.  On the first run, you will be prompted to select a valid script file (`.ini`).
3.  The application will initialize based on the parameters defined in the script.
4.  Use the top menu bar to access Reports, Permissions, and other tools.

## Folder Structure

The source code is organized as follows:

* `src/Core`: Backend logic, script parsing, and utilities.
* `src/Gui`: Main window and primary UI logic.
* `src/Widgets`: Custom reusable widgets (Calendar, etc.).
* `src/Dialogs`: Popups for specific tasks (Line Writing, Clothing, etc.).

## Disclaimer

This software is provided "as is", without warranty of any kind. Use at your own risk.

## Licensing

This application uses the **Qt Toolkit** (http://www.qt.io), licensed under the **LGPLv3**.
* The source code for Qt can be obtained from [http://download.qt.io](http://download.qt.io).
* The Qt libraries used are dynamically linked. Users may substitute them with their own compatible versions of the Qt libraries.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/D1D11DFKBC)
