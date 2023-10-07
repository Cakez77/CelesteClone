#!/bin/bash

timestamp=$(date +%s)

defines="-DENGINE"
libs="-luser32 -lopengl32 -lgdi32 -lole32"
warnings="-Wno-writable-strings -Wno-format-security -Wno-deprecated-declarations -Wno-switch"
includes="-Ithird_party -Ithird_party/Include"

clang++ $includes -g src/main.cpp -oschnitzel.exe $libs $warnings $defines

rm -f game_* # Remove old game_* files
clang++ -g "src/game.cpp" -shared -o game_$timestamp.dll $warnings $defines
mv game_$timestamp.dll game.dll