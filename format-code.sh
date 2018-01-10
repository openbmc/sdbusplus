#!/bin/bash

# This script reformats source files using the clang-format utility.
#
# Files are changed in-place, so make sure you don't have anything open in an
# editor, and you may want to commit before formatting in case of awryness.
#
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Formatting code under $DIRECTORY/"
find . -regextype sed -regex ".*\.[hc]\(pp\)\?" -print0 | xargs -0 "clang-format-3.9" -i
find "." \( -name '*.h' -or -name '*.c' -or -name '*.mak' -or -name '*.sh' \) -print0 | xargs -0 dos2unix

