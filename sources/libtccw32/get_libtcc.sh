#!/bin/sh

git clone --depth 1 https://github.com/movsb/libtccw32.git
mv libtccw32/* .
rm -rf libtccw32/
