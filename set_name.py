#!/usr/bin/env python3
import argparse
import serial
import serial.tools.list_ports
import time
import sys

def list_ports():
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("No serial ports found.")
        return
    print("Available serial ports:")
    for port in ports:
        print(f"  {port.device} - {port.description}")

def set_name(port, name):
    try:
        # Open serial port
        # Typical ESP32 baudrate is 115200
        ser = serial.Serial(port, 115200, timeout=2)
        
        # Give it a moment to initialize (some ESP32s reset on connect)
        time.sleep(2)
        
        # Clear buffers
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Send command
        command = f"name={name}\n"
        print(f"Sending: {command.strip()}")
        ser.write(command.encode('utf-8'))
        
        # Read response
        # The device should print "Device name updated and saved to flash: ..."
        # We'll wait a bit for the response
        time.sleep(0.5)
        while ser.in_waiting:
            line = ser.readline().decode('utf-8').strip()
            if line:
                print(f"Device: {line}")
            
        ser.close()
        print("Done.")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description="Set the device name of a Light Instrument Controller via Serial.")
    parser.add_argument("name", nargs="?", help="The new device name to set.")
    parser.add_argument("-p", "--port", help="The serial port to use (e.g., /dev/ttyUSB0 or COM3).")
    parser.add_argument("-l", "--list", action="store_true", help="List available serial ports.")

    args = parser.parse_args()

    if args.list:
        list_ports()
        return

    if not args.name:
        if len(sys.argv) == 1:
            parser.print_help()
            print("\nError: Device name is required.")
        else:
            print("Error: Device name is required unless using --list.")
        sys.exit(1)

    if not args.port:
        ports = serial.tools.list_ports.comports()
        if len(ports) == 1:
            args.port = ports[0].device
            print(f"Auto-detected port: {args.port}")
        elif len(ports) > 1:
            print("Multiple serial ports found:")
            for p in ports:
                print(f"  {p.device} - {p.description}")
            print("\nError: Please specify a port with -p.")
            sys.exit(1)
        else:
            print("Error: No serial ports found. Make sure your device is connected.")
            sys.exit(1)

    set_name(args.port, args.name)

if __name__ == "__main__":
    main()
