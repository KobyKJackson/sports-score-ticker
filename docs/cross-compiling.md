# Cross-Compiling for Raspberry Pi

This project can be built on your workstation and pushed to the Pi without compiling the C++ display binary on-device.

## 1. Install a Pi cross toolchain

Set `PI_TOOLCHAIN_PREFIX` to the Linux target triplet for your Pi:

- 64-bit Raspberry Pi OS: `aarch64-linux-gnu`
- 32-bit Raspberry Pi OS: `arm-linux-gnueabihf`

If your toolchain needs a sysroot, export `PI_SYSROOT` to the Raspberry Pi root filesystem path.

Example:

```bash
export PI_TOOLCHAIN_PREFIX=aarch64-linux-gnu
export PI_SYSROOT=$HOME/sysroots/raspberry-pi
```

## 2. Cross-build the LED binary

```bash
./deploy/cross-build-pi.sh
```

This builds `display/libs/rpi-rgb-led-matrix/lib/librgbmatrix.a` with the cross compiler, then builds `display/build-pi/led_ticker` using `toolchains/raspberry-pi.cmake`.

## 3. Push to the Raspberry Pi

```bash
./deploy/push-to-pi.sh pi@raspberrypi.local /home/pi/sports-score-ticker
```

The deploy script:

- syncs the repository to the Pi with `rsync`
- uploads `display/build-pi/led_ticker`
- runs `sudo SKIP_BUILD=1 ./deploy/install.sh`

`SKIP_BUILD=1` tells the installer to keep the prebuilt binary instead of running `cmake` and `make` on the Pi.

## Notes

- Python dependencies are still installed on the Pi by `deploy/install.sh`.
- If you switch target architecture, rebuild with a clean cross toolchain and rerun `./deploy/cross-build-pi.sh`.
- The display binary must match the Pi OS architecture and libc in your sysroot/toolchain.
