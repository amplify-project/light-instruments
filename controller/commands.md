# Light Instrument Controller Commands

This document describes the available commands that can be sent to the Light
Instrument Controller via ESP-NOW using the `CommandPacket` structure.

## General Commands

### `set`

Sets a static color and brightness for the targeted port.

- **Parameters**: `r,g,b,brightness`
- **Behavior**: Updates the color of a running animation if one is active. If
  no animation is running, it sets the strip to a solid color and updates the
  strip's base brightness. All values have to be between 0-255.

### `setColor`

Sets a static color for the targeted port.

- **Parameters**: `r,g,b`
- **Behavior**: Updates the color of a running animation if one is active. If
  no animation is running, it fills the strip with the specified color. Values
  have to be between 0-255.

### `setBrightness`

Updates the base brightness of the targeted port.

- **Parameters**: `brightness` (0-255)
- **Behavior**: Adjusts the intensity of the LEDs without interrupting any
  currently running animation.

### `setLED`

Sets a specific range of LEDs to a certain color.

- **Parameters**: `r,g,b,offset,numleds`
- **Behavior**: Sets `numleds` starting from `offset` to the specified RGB color.
  This command **interrupts and stops** any currently running animation on the
  targeted port to prevent the animation from overwriting the manual changes.
  Values for `r,g,b` are 0-255. `offset` and `numleds` are clamped to the
  strip's length.

### `stop`

Stops all activity on the targeted port.

- **Parameters**: None
- **Behavior**: Halts any active animation and turns off all LEDs for the
  specified port.

## Animation Commands

All animations run independently per port and can be updated in real-time
using `set` or `setColor`.

### `rainbow`

Starts a cycling rainbow animation.

- **Parameters**: `deltaHue` (optional)
- **Default**: 5
- **Behavior**: Gradually cycles colors across the strip. A higher `deltaHue`
  creates a more "compressed" rainbow.

### `glitter`

Starts a sparkling glitter animation over a background color.

- **Parameters**: `r,g,b,duration`
- **Optional**: `duration` can be omitted (defaults to 0).
- **Behavior**: Randomly flashes white pixels over the specified background
  color. If `duration` is `0`, the animation runs indefinitely. The duration is
  given in milliseconds.

### `pulse`

Starts an ADSR (Attack, Decay, Sustain, Release) pulse animation.

- **Parameters**: `r,g,b,attack,decay,sustain,release`
- **Behavior**: Smoothly pulses the color through four stages of timing (in
  milliseconds).

### `comet`

Starts a moving comet effect.

- **Parameters**: `r,g,b,speed`
- **Behavior**: Sends a "comet" of color down the strip. The `speed` parameter
  determines how quickly the tail fades (in milliseconds).

### `breathe`

Starts a smooth breathing/pulsing animation.

- **Parameters**: `r,g,b,bpm`
- **Behavior**: Pulses the color intensity following a sine wave at the
  specified Beats Per Minute (BPM).

### `fire`

Starts an organic fire flicker effect.

- **Parameters**: `r,g,b,intensity`
- **Behavior**: Simulates a flickering fire using a heat-map algorithm based on
  the specified color and spark intensity (0-255).

## Device Configuration

The device name is used to target specific controllers in a multi-device setup. You can set a persistent name via the USB Serial connection.

### Setting the Device Name

A Python script `set_name.py` is provided in the parent directory to simplify this process.

**Requirements**:

- Python 3
- `pyserial` library (`pip install pyserial`)

**Usage**:

```bash
# Set a name (auto-detects port if only one is connected)
./set_name.py my-device-name

# Specify a port
./set_name.py my-device-name -p /dev/ttyUSB0

# List available ports
./set_name.py --list
```

The name is saved to the device's non-volatile storage (NVS) and will persist across reboots and firmware updates.
