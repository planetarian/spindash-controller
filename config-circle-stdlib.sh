#! /bin/bash

cd circle-stdlib

./configure -r 4 \
    -o MULTICORE=1 \
    -o USEFLASHY=3 \
    -o SERIALPORT=/dev/ttyS17 \
    -o FLASHBAUD=3000000 \
    -o USERBAUD=3000000 \
    -o REBOOTMAGIC=tAgHQP3Lw2NZcW8Uru7jnf \
    -o SDCARD=~/spindash-controller/libs/circle/boot/sd
    # \
    #-o STDLIB_SUPPORT=3

cat >> Config.mk<< EOF

# adjust according to environment
USEFLASHY = 3
SERIALPORT = /dev/ttyS17
FLASHBAUD = 3000000
USERBAUD = 3000000
REBOOTMAGIC = tAgHQP3Lw2NZcW8Uru7jnf
SDCARD = ~/spindash-controller/circle-stdlib/libs/circle/boot/sd
EOF