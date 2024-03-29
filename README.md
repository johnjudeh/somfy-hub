# Somfy Hub

## Introduction

This repo creates a home automation hub for controlling [Somfy](https://www.somfy.co.uk) devices,
including electric blinds. The goal of the code is to allow the user to control multiple Somfy
devices through a variety of methods, ultimately including Apple Home.

## Motivation

Somfy devices come with a remote control that can control them. This remote control uses a
proprietary protocol called [RTS](https://www.somfy.co.uk/help-me-choose/rts-products) to
communicate with Somfy devices. Due to its proprietary nature, the only recommended way to make
the devices smart is to use the unnecessarily expensive
[TaHoma switch](https://www.somfy.co.uk/products/1870600/tahoma-switch) as a hub.

This project aims to create a cost-effective replacement for that hub.

## Previous work

Despite the fact that RTS is proprietary and there is no documentation on how it works, some clever
person has figured out how the protocol works using observation and shared it on
[PushStack](https://pushstack.wordpress.com/somfy-rts-protocol/).

This project was forked from a
[Github project by Nickduino](https://github.com/Nickduino/Somfy_Remote) which shared the code
required to communicate in RTS based on the PushStack explanation. This project adapts Nickduino's
work to control many Somfy devices and ultimately aims to provide access to those devices on
Apple Home.

## Hardware

To get it working, you need the following things:

- An ESP32 development board, or any other micro controller with WIFI
- An RF transmitter communicating at a frequency of 433.42 MHz

The ESP32 is really cheap and easy to come by. The RF transmitter is also very cheap **BUT** one
that transmits at 433.42 MHz is not easy to come by. Most transmit at 433.92 MHz. Fortunately,
you can buy a RF transmitter and 433.42 MHz crystal separately and replace the crystal yourself.
This is not too hard to do but requires you to de-soldier the old crystal and re-soldier the new
one.

You'll need to then connect your ESP32 to a power source, and connect the RF transmitter on GPIO
pin `13`. Alternatively, you can choose a different pin and change the macro variable
`RF_DATA_OUT_PIN`.

The sketch also accommodates for adding hardware buttons to create a multi-channel Somfy remote.

## Software

Once the hardware is setup properly, the Arduino Sketch in this repo can be uploaded onto your
ESP32. Once uploaded successfully, you can link the hub to each of your devices and control them
either by sending commands through the Serial or clicking the hardware buttons associated
to each command.

The valid commands are like any other Somfy remote which include:

- **p** - programme
- **u** - up
- **d** - down
- **s** - stop
