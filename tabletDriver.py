#!/usr/bin/env python3
'''
Meant to run on your PC.
Receives data generated by rmServerWacomInput.py (or the c_implementation).
This data is passed through to the evdev on linux,
emulating the original tablet input (which should support all its features).

Does the same as tabletDriver.c !
'''


import socket
import struct
from sys import argv
from evdev.uinput import UInput
from evdev.device import AbsInfo
from evdev import ecodes

WACOM_WIDTH = 15725   # Values just checked by drawing to the edges
WACOM_HEIGHT = 20967  # ↑

# Source: https://github.com/canselcik/libremarkable/blob/master/src/input/wacom.rs
EV_SYNC = 0
EV_KEY = 1
EV_ABS = 3
WACOM_EVCODE_PRESSURE = 24  # = ecodes.ABS_PRESSURE
WACOM_EVCODE_DISTANCE = 25  # = ecodes.ABS_DISTANCE
WACOM_EVCODE_XTILT = 26     # = ecodes.ABS_TILT_X
WACOM_EVCODE_YTILT = 27     # = ecodes.ABS_TILT_Y
WACOM_EVCODE_XPOS = 0       # = ecodes.ABS_X
WACOM_EVCODE_YPOS = 1       # = ecodes.ABS_Y

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client.connect((('10.11.99.1' if len(argv) == 1 else argv[1]), 33333))

# See https://python-evdev.readthedocs.io/en/latest/apidoc.html#evdev.device.AbsInfo.resolution
# Resolution = max(20967, 15725) / (21*10)  # Height of display is 21cm. Format is units/mm. => ca. 100 (99.84285714285714)
# Tilt resolution = 12400 / ((math.pi / 180) * 140 (max angle)) (Format: units/radian) => ca. 5074 (5074.769042587292)

# BTN_TOOL_PEN == 1 means that the pen is hovering over the tablet
# BTN_TOUCH == 1 means that the pen is touching the tablet

# This WONT'T WORK with libinput since the resolution is never actually set.
# This is fixed in the c-version but doesn't quite work yet, too.
capabilities = {
	ecodes.EV_KEY: [ecodes.BTN_TOOL_PEN, ecodes.BTN_TOUCH],
	EV_ABS: [
		(WACOM_EVCODE_PRESSURE, AbsInfo(value=0, min=0, max=4095, fuzz=0, flat=0, resolution=0)),
		(WACOM_EVCODE_DISTANCE, AbsInfo(value=0, min=0, max=110, fuzz=0, flat=0, resolution=0)),
		(WACOM_EVCODE_XTILT, AbsInfo(value=0, min=0, max=12400, fuzz=0, flat=6200, resolution=5074)),
		(WACOM_EVCODE_YTILT, AbsInfo(value=0, min=0, max=12400, fuzz=0, flat=6200, resolution=5074)),
		(WACOM_EVCODE_XPOS, AbsInfo(value=0, min=0, max=WACOM_HEIGHT, fuzz=0, flat=0, resolution=100)),
		(WACOM_EVCODE_YPOS, AbsInfo(value=0, min=0, max=WACOM_WIDTH, fuzz=0, flat=0, resolution=100))
	]
}

# Prod and Vend Ids are random as well as phys !
with UInput(events=capabilities, name='reMarkableTablet-FakePen', vendor=0x0002, product=0x02FE, phys='fake-tablet-input') as inputDevice:
	print('Running...')

	while True:
		evDevType, evDevCode, evDevValue = struct.unpack('HHi', client.recv(8))
		inputDevice.write(evDevType, evDevCode, evDevValue)
