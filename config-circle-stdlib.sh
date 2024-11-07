#! /bin/bash


cd circle-stdlib

# apply multi-core and device ID edits
sed -i -r 's/\/\/(#define ARM_ALLOW_MULTI_CORE)/\1/g' libs/circle/include/circle/sysconfig.h
sed -i -r 's/(#define USB_GADGET_VENDOR_ID\s+0x)0000/\1f0d4/g' libs/circle/include/circle/sysconfig.h

./configure -r 4 \
    -o MULTICORE=1 \
    -o STDLIB_SUPPORT=3 \
    -o USEFLASHY=3 \
    -o SERIALPORT=/dev/ttyS17 \
    -o FLASHBAUD=3000000 \
    -o USERBAUD=3000000 \
    -o REBOOTMAGIC=tAgHQP3Lw2NZcW8Uru7jnf \
    -o SDCARD=~/spindash-controller/circle-stdlib/libs/circle/boot/sd
    

# i don't have a clue what i'm doing so lol duplicate stuff
cat >> Config.mk<< EOF

# adjust according to environment
MULTICORE = 1
STDLIB_SUPPORT = 3
USEFLASHY = 3
SERIALPORT = /dev/ttyS17
FLASHBAUD = 3000000
USERBAUD = 3000000
REBOOTMAGIC = tAgHQP3Lw2NZcW8Uru7jnf
SDCARD = ~/spindash-controller/circle-stdlib/libs/circle/boot/sd
EOF