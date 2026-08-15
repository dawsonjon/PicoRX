#!/usr/bin/env bash

# Format all C/C++ files in the repo using clang-format
clang-format -i $(git ls-files "src/*.c" "src/*.cpp" "src/*.h" "src/*.hpp")
clang-format -i $(git ls-files "eibi/*.c" "eibi/*.cpp" "eibi/*.h" "eibi/*.hpp")
clang-format -i $(git ls-files "codecs/*.c" "codecs/*.cpp" "codecs/*.h" "codecs/*.hpp")
echo "✨ All files formatted."

