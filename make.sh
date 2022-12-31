#!/bin/sh

set -xe
clang -Wall -Wextra -pedantic main.c -o out/vt -I/opt/local/include -I/opt/local/include/editline -L/opt/local/lib -ledit
rm -rf out/*.dSYM
