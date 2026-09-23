# Light Instruments

This repository contains all the ESP-32 code powering the light instruments,
the LED controllers and the wireless receiver.

The system is fully wireless through ESP-Now. The `receiver` sends out a
device discovery message every 10 seconds on its broadcast channel. To join the
network, sensor and actuator nodes have to respond to this message with their
unique name and are added to the receiver's peer list upon success.

## Instruments

- `key_instrument`: Reads digital signals on ports `D1`, `D2` and `D3` and
  sends a message to the receiver if a state change (i.e. a button press) is
  observed.
- `maracas`: Reads a digital signal on port `D3` and sends a message to the
  receiver if a state change is observed.
- `maracas_accel`: Reads acceleration values on the X, Y and Z-axis from a
  MPU6050 or MMA8451 accelerometer, performs peak detection and sends a message
  to the receiver if a peak (i.e. a shake) is detected.
- `multiplexer`: Reads digital signals from up to 8 ports. The number of active
  ports is controlled by a physical 3-bit DIP-switch, where 0 on the switch
  corresponds to all 8 ports being active. The ports are accessed by setting
  their address on a CD4051BE multiplexer chip. Messages to the receiver are
  sent when a state change on a port is observed.
- `rainstick`: Reads an analog signal on port `A3`, which a photodiode is
  attached to. Sends a signal to the receiver if a value change above a
  configurable threshold is observed.
- `touch_instrument`: Reads a capacitive touch value from ports `D1`, `D2` and
  `D3` and maps them to a 0-1023 range, taking into account measured baseline
  calibration minima for each port. These are then mapped to a binary touch
  state using a configurable threshold. A message to the receiver is sent, if
  a state change is observed.
- `vibration_detector`: Reads an analog value from port `A1`, corresponding
  to the strength of vibration detected by the vibration detection circuit.
  Performs filtering and sends a message to the receiver if a change in
  vibration above a certain threshold is detected.
- `vibration_detector2`: Uses the same hardware as `vibration_detector`, but
  performs additional filtering and only sends a single message to the receiver
  if vibration above a given threshold is detected.

Any newly flashed device needs to be given a unique name before it can join
the network. This can be done through the serial interface, by sending the
command `name=[DEVICE_NAME]`. The WiFi channel to use can be configured through
the command `channel=[CHANNEL_NUM]`. In order to communicate, the receiver and
the instruments need to be on the same WiFi channel.

Upon boot, each instrument flashes the builtin LED on the microcontroller to
signal boot start. Once the ports are set up and configured and the relay has
been discovered, the builtin LED will turn on. This indicates that the device
is ready and can communicate with the relay.

## Common Library

The folder `common/` contains a C++ library used by all light instruments. It
provides methods for relay discovery, ping handling and data transmission to
the receiver relay.

## LED Controllers

The code for the LED strip controllers if found in `controller/`. Each
controller can host up to four individual LED strips. Upon boot, it waits for
device discovery messages from the receiver and then joins the network using
its unique name. As with the Light Instruments, this name needs to be
configured through the serial interface using the command `name=[DEVICE_NAME]`.
In addition to that, the controller supports the following commands through the
serial interface:

- `channel=[CHANNEL_NUM]`: Configure the WiFi channel number. In order to
  communicate, the controller needs to be on the same channel as the receiver.
- `numstrips=[NUMBER_OF_STRIPS]`: The number of LED strips this controller is
  controlling (1-4).
- `numleds=[NUMBER_OF_LEDS]`: The number of LEDS for each strip. This acts as
  a global fallback value. The number of LEDS can also be configured on a
  per-strip basis using the command `numleds_[0-3]=[NUMBER_OF LEDS]`.
- `status`: Prints the current configuration values stored in non-volatile
  storage.

After changing the configuration, the microcontroller needs to be rebooted for
the changes to take effect.

The controller supports a series of animations that can be executed on the LED
strips. The animations are triggered through commands and parameters sent out
the the receiver.

## Receiver

The code for the receiver is found in the folder `receiver/`. This device is
plugged into a USB port and acts as a relay between the sensors (i.e. the
Lights Instruments) and the actuators (i.e. the LED strip controllers). It
receives events from sensors and sends them to the Light Instrument Node
Editor application through the serial connection for processing and also
forwards light commands received through the serial to actuators.
