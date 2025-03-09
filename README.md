# Tank Game for Raspberry Pi 4

A Yocto project that includes a tank game for Raspberry Pi 4.

## Description

This project contains a custom Yocto layer (`meta-tank-game`) that adds a tank game to a Yocto image for Raspberry Pi 4. The game runs automatically at system startup.

https://github.com/user-attachments/assets/08f128c0-8952-45dd-abb7-5aa00917ba57


## Requirements

- Yocto Project (tested with version kirkstone)
- The meta-raspberrypi layer

## Installation

1. Clone the Poky repository:
```sh
git clone git://git.yoctoproject.org/poky -b kirkstone
```

2. Clone the meta-raspberrypi layer:
```sh
cd poky
git clone git://git.yoctoproject.org/meta-raspberrypi -b kirkstone
```

3. Clone this repository
```sh
git clone https://github.com/username/tank-game-yocto-project.git
cp -r tank-game-yocto-project/meta-tank-game .
```

4. Initialize the build environment:
```sh
source oe-init-build-env build
```

5. Configure the build:
Add the required layers:
```sh
bitbake-layers add-layer ../meta-raspberrypi
bitbake-layers add-layer ../meta-tank-game
```
Edit `conf/local.conf` as per the example in the repository

6. Build the image:

bitbake core-image-minimal


## Configuration

Example configuration for `conf/local.conf`:
```sh
MACHINE = "raspberrypi4-64"
GPU_MEM = "128"
IMAGE_INSTALL:append = " tank-game"
ENABLE_UART = "1"
DISTRO_FEATURES:append = " systemd"
VIRTUAL-RUNTIME_init_manager = "systemd"
VIRTUAL-RUNTIME_initscripts = ""
DISTRO_FEATURES_BACKFILL_CONSIDERED = "sysvinit"
```
