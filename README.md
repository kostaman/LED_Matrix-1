# LED_Matrix

LED Matrix Driver based on [Colorlight 5A-75B](http://www.colorlight-led.com/product/colorlight-5a-75b-led-display-receiving-card.html) LED receiver card by sending L2 Ethernet packets. Inspired by [Falcon Player](https://github.com/FalconChristmas/fpp/blob/master/src/channeloutput/ColorLight-5a-75.cpp) and currently only targets Linux based systems. (Pi, BeagleBone, PC, etc.) However due to the interface of the receiver card this could be done on pretty much anything capable of Ethernet.

Firmware images are products of ColorLight and I am just saving them off. They are exactly the same as the ones provided by [LEDUpgrade 3.6](https://www.colorlightinside.com/Products/Software/37_143.html). Configuration files were produced by [LEDVision 8.0](https://www.colorlightinside.com/Products/Software/37_31.html).

Current logic builds 16x128 LED Matrix, however configuration of Receiver card is 64x32 for performance reasons. This is easily corrected with a simple mapper function. Honestly I would not expect this to have been supported. These are well designed to support signage market, which provides functionality that can be used here.

The receiver card supports:
- gamma correction
- memory map
- current gain (depending on panel)
- PWM drivers (depending on panel)
- high refresh (depending on configuration)
- high quality (depending on configuration)
- low cost
- some mapping
- low memory usage
- offload acceleration
- wide voltage range (300mA average power draw)
- portable/streamlined interface
- large number of pixels
- large number of IO connectors
- etc 

Linux logic uses IO vectors in scatter gather configuration to support high performance, lower memory usage, and streamlined interface. Rough performance estimates show PC as very high performance. Pi has good performance but may struggle in certain applications. Pi 2 is about 2.75 times faster than Pi.

VLAN enables channels via PoE switch which would enable a number of different applications using COTS solutions. Receiver cards also support chaining as an option. Physical LANs is also an alternative using multiple NICs.

Note this will work with PWM/MM panels. You need to check the receiver card support, however this card has very good support. You may need to use a different firmware and configuration.

## About
This logic works off shared memory map created by daemon. This enables other languages on the system to use the logic without inheriting/requiring super user. Shared memory map is creating in /tmp and is assumed to be a RAM disk or something like it. (This may not be desirable for every use case.)

Shared memory was choosen to avoid overhead of other methods. There is a command interface built into the first four bytes of memory. The daemon will poll this periodically to activate Matrix functions. Double buffering may come later however currently logic should wait for the command to be reset back to zero before proceeding. Current implementation is blocking to prevent writing while updating issues. (Also due to laziness.)

Note there is no protection against race conditions and hazards on shared memory. If multiple processors grab there could be problems, however this is not expected to be very common. TCP logic currently processes connections serially one at a time to prevent this issue and possible resource usage issues. Note this is still a work in progress and very lazy.

Examples of language portability is shown with trigger programs. Currently Matrix class is only implemented on Linux, inorder to use on other systems this would need to be reimplemented or use network daemon running on Linux server. Note other languages could wrap up command interface into its own class wrapper.

This code base is very straight forward, and this logic is fairly light weight. This is due to the significant amount of offloading provided by the ColorLight 5A-75B. Note configuration complexity is also passed off to application logic and ColorLight configuration software. This simplifies the code down to basically a wrapper/interface logic for higher level logic.

Therefore converting this to something not based in Linux would be straightforward, given it can generate L2 packets.

## Building
Daemon:
```bash
g++ -O3 Matrix.cpp network.cpp main.cpp -o Matrix -lpthread
```
Trigger (C++):
```bash
g++ -O3 trigger.cpp -o trigger
```
Trigger Remote (C):
```bash
gcc -O3 trigger_remote.c -o trigger_remote
```

## Running
Daemon: (Assuming your ethernet interface is "ens33" and the receiver card is configured for 64x32.)
```bash
sudo ./Matrix "ens33" 64 32
```
Trigger (C++):
```bash
./trigger
```
Trigger (Groovy):
```bash
groovy trigger.groovy
```
Trigger (Python):
```bash
python3 trigger.py
```
Trigger Remote (C):
```bash
./trigger_remote 127.0.0.1
```

## Files
The base of this repo is in Matrix.h and Matrix.cpp. If looking to see how it works or port this to another implementation/platform look there.

For the daemon look at main.cpp (trigger protocol) and network.cpp (TCP protocol). This logic uses Matrix.h and Matrix.cpp.

For the trigger programs look at the respective implementation. Note these only work with daemon. These could be rewritten to support define a complete class which scretely ties into daemon locally or remotely. NOte the performance impact of this is unknown.

## Mapping
Multiplex mapping is handled by the receiver card, however pixel mapping is handled by application logic. Currently the trigger programs do not bother with this. A little piece exists in Matrix.h and Matrix.cpp for supporting this with the map virtual method. Note that is only used in set_pixel not in set_pixel_raw. (Raw is used by network.cpp)

This is kind of important for CC based panels which need a lot of serial bandwdith for high quality and refresh. One simple way to get this is to use multiple IO connectors provided by the receiver card. This causes the panels to be mapped vertically inside of horizontally, which is easily correct with a couple lines of code.

Other applications also likely exist which may want this. Nice thing here is you are free to use as much processing power as you want. You can run this on a computer if you need more. So you can be pretty lazy potentially. The receiver card will if nothing else continue to show the last frame it got without any issues.
