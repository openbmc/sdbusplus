#!/bin/bash

# This script reformats source files using the clang-format utility.
#
# Files are changed in-place, so make sure you don't have anything open in an
# editor, and you may want to commit before formatting in case of awryness.
#
# This must be run on a clean repository to succeed

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Formatting code under $DIR/"
find . -regextype sed -regex ".*\.[hc]\(pp\)\?$" -print0 | xargs -0 "clang-format-3.9" -i

git --no-pager diff --exit-code
