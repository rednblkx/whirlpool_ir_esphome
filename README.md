# Whirlpool AC IR ESPHome Component

### Custom ESPHome component based on the [implemention](https://github.com/esphome/esphome/tree/dev/esphome/components/whirlpool) already present on the [esphome](https://github.com/esphome/esphome) repository

## Features:

- 6th sense implemented properly with offsets as on the remote
- Ignore received data if state was just transmitted a sec ago so to avoid double state change (off->cool->off)
- Added a switch to disable the transmitter and adjust the state without triggering the AC in case receiver hasn't picked up the data from the remote


## Usage

Edit [``secrets.yaml``](secrets.yaml) accordingly, compile and upload it to your favorite ESP device 😁
