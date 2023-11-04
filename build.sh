#!/bin/bash

defines="-DENGINE"
warnings="-Wno-writable-strings -Wno-format-security -Wno-deprecated-declarations -Wno-switch"
includes="-Ithird_party"

timestamp=$(date +%s)
flags="${flags:--g}"

if [[ "$(uname)" == "Linux" ]]; then
    echo "Running on Linux"
    libs="-lX11 -lGL -lfreetype"
    outputFile=schnitzel
    queryProcesses=$(pgrep $outputFile)

    # fPIC position independent code https://stackoverflow.com/questions/5311515/gcc-fpic-option
    rm -f game_* # Remove old game_* files
    clang++ $flags "src/game.cpp" -shared -fPIC -o game_$timestamp.so $warnings $defines
    mv game_$timestamp.so game.so

elif [[ "$(uname)" == "Darwin" ]]; then
    echo "Running on Mac"

    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found. Please install Homebrew and try again."
        exit 1
    fi

    # Build Game Library
    rm -f -R *.dylib *.dSYM  # Remove old build files
    clang++ $flags -dynamiclib "src/game.cpp" -o game_$timestamp.dylib $warnings $defines
    mv game_$timestamp.dylib game.dylib
    mv game_$timestamp.dylib.dSYM game.dylib.dSYM

    # Game Engine Compiler Settings on Mac
    HOMEBREW_CELLAR=${HOMEBREW_CELLAR:-/usr/local/Cellar} # Homebrew defaults to /usr/local/Cellar 
    libs="-framework Cocoa -framework OpenGL -L${HOMEBREW_CELLAR}/glfw/3.3.8/lib -lglfw -L${HOMEBREW_CELLAR}/freetype/2.13.2/lib -lfreetype"
    includes="-Ithird_party -I${HOMEBREW_CELLAR}/glfw/3.3.8/include -I${HOMEBREW_CELLAR}/freetype/2.13.2/include/freetype2"
    outputFile=schnitzel
else
    echo "Running on Windows"
    libs="-luser32 -lopengl32 -lgdi32 -lole32 -Lthird_party/lib -lfreetype.lib"
    outputFile=schnitzel.exe
    queryProcesses=$(tasklist | grep $outputFile)

    rm -f game_* # Remove old game_* files
    clang++ $flags "src/game.cpp" -shared -o game_$timestamp.dll $warnings $defines
    mv game_$timestamp.dll game.dll
fi

processRunning=$queryProcesses

if [ -z "$processRunning" ]; then
    echo "Engine not running, building main..."
    clang++ $includes $flags "src/main.cpp" -o $outputFile $libs $warnings $defines
else
    echo "Engine running, not building!"
fi