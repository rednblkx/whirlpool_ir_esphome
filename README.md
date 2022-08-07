# Whirlpool AC IR ESPHome Component

### Custom ESPHome component based on the [implemention](https://github.com/esphome/esphome/tree/dev/esphome/components/whirlpool) already present on the [esphome](https://github.com/esphome/esphome) repository

## Features:

- 6th sense implemented properly with offsets as on the remote
- Ignore received data if state was just transmitted a sec ago so to avoid double state change (off->cool->off)
- Added a switch to disable the transmitter and adjust the state without triggering the AC in case receiver hasn't picked up the data from the remote


## Usage

1. Edit [``secrets.yaml``](secrets.yaml) accordingly
2. Edit the ``board`` model ([see here a list](https://registry.platformio.org/platforms/platformio/espressif8266/boards))
3. Edit the ``pin`` for the ``remote_transmitter`` and ``remote_receiver`` in [``climate_ir.yaml``](climate_ir.yaml) to match your configuration
4. Compile and upload it to your favorite ESP device 😁
