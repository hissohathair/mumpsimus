#!/bin/sh

aclocal --install -I m4 &&
  autoconf &&
  autoreconf -fi &&
  automake --add-missing --copy &&
  ./configure "$@"
