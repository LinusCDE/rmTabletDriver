# rmTabletDriver

This userspace driver for linux is able to simulate the real tablet input from your reMarkable Paper Tablet on your Linux PC (or VM).

There are two files. `tabletDriver.c` and `tabletDriver.py`. They both do the same thing. Choose as you wish.

This is the client side. You can find the server side (to be run on the reMarkable) in [rmWacomToMouse](https://github.com/LinusCDE/rmWacomToMouse).

# Problems

Currently the driver won't work with the libinput driver. This driver is used on Wayland and often replaces the evdev xinput driver on xorg, providing nearly the same support as the evdev xinput driver.

If the command `libinput` exists, you'll likely have this driver and probably not the evdev xinput driver.

When developing on my laptop (which used the libinput driver) I encountered two issues when checking my fake device using `libinput list-devices`:

- It was missing a stylus button
- There was no resolution found (which was given in tabletDriver.py)

It turned out, that the pydev library wasn't able to send the resolution (probably using an old way for setting abs info). This is fixed as of version 1.2.0.

Thus the tabletDriver.c was created. Doing the same but having the ability to specify resolution (can be seen with the `evtest` command).

CURRENT PROBLEM: Since the libinput driver was mainly made for Wayland, the xinput driver isn't the best. So the virual tablet device is added as a keyboard. I currently don't know why and how to fix it. But I haven't given up yet.


## libinput workaround

To still use the tablet, you can install the evdev xinput driver alongside the libinput driver.
On most distros it is called something alongside of `xorg-input-evdev` (on Arch it is `xf86-input-evdev`).

This may cause some devices to STOP WORKING (in my case the touchpad on my laptop).

To fix this, you either remove the evdev driver again or create a file in `/etc/X11/xorg.conf.d/` and call it for example `90-libinput_override.conf`:

```
Section "InputClass"
        Identifier "libinput override fix for Device X"
        MatchProduct "<PRODUCTNAME>"
        Driver "libinput"        
```

**DISCLAIMER**: Mistyping something in the file (except the product name in the quotes) can lead to the failure of all graphical UIs (greeter and desktop) and seem like you bricked you computer!

To get the product name you can install `evtest`, run `evtest /dev/input/event0`, check if input on your failed device produces any output in the terminal, and if not cancel (Ctrl+C) and try with the next number (event1) and so on.
When you found the device, you run the command again and will find the product name in the first few lines of output.

To apply the changes reboot your computer.


## Tablet not working with evdev xinput driver

Create a file called `/etc/X11/xorg.conf.d/10-evdev.conf` and add [this content](https://gist.github.com/Leonidas-from-XIV/4306072).


# Enabling the Tablet in GIMP

In order to work in GIMP, the device must get enabled under `Edit -> Input devices`.


# Calibrating

It also is a good idea to calibrate the tablet using xinput_calibrator because the tablet screen will be streched to match your screen. That will result in a wrong aspect ratio.

When calibration using xinput_calibrator use the pdf, open it on the reMarkable and touch the points there.

(I essentially just recorded the calibration screen, took a screenshot, changed the colors, added borders, and created a pdf from it.)

# Credits

Huge thanks to benthor's [HuionKamvasGT191LinuxDriver](https://github.com/benthor/HuionKamvasGT191LinuxDriver) (found 
[here](https://docs.krita.org/en/reference_manual/list_supported_tablets.html)) as it gave me the final hint on how to get the driver to work with Krita!
