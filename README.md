# LED_Matrix

LED Matrix Driver based on [Colorlight 5A-75B](http://www.colorlight-led.com/product/colorlight-5a-75b-led-display-receiving-card.html) and [Colorlight 5A-75E](http://www.colorlight-led.com/product/colorlight-5a-75e-led-display-receiving-card.html) LED receiver card by sending L2 Ethernet packets. Inspired by [Falcon Player](https://github.com/FalconChristmas/fpp/blob/master/src/channeloutput/ColorLight-5a-75.cpp) and currently only targets Linux based systems. (Pi, BeagleBone, PC, etc.) However due to the interface of the receiver card this could be done on pretty much anything capable of Ethernet.

Firmware images are products of ColorLight and I am just saving them off. They are exactly the same as the ones provided by [LEDUpgrade 3.6](https://www.colorlightinside.com/Products/Software/37_143.html). Configuration files were produced by [LEDVision 8.0](https://www.colorlightinside.com/Products/Software/37_31.html).

Current logic builds 16x128 LED Matrix, however configuration of Receiver card is 64x32 for performance reasons. This is easily corrected with a simple mapper function. Honestly I would not expect this to have been supported. These are well designed to support signage market, which provides functionality that can be used here.

The receiver card supports:
- gamma correction
- memory map
- current gain (depending on panel)
- PWM drivers (depending on panel, see [below](https://github.com/daveythacher/LED_Matrix/blob/main/README.md#pwm-support))
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

#### PWM Support
Note this will work with PWM/MM panels. You need to check the receiver card support, however this card has very good support. You may need to use a different firmware and configuration. 

In my testing there is a possible issue with PWM firmware with MBI5153 based panels. Changing images with Ethernet can cause small glitches randomly/periodically. LED panel current is very steady when Ethernet is not connected to receiver card. When receiver card is playing the same image over and over things work fine. So for static frame send periodically over Ethernet should work. Ethernet connected without traffic does not cause an issue. No issues have been found with non-PWM panels and non-PWM firmware.

Further testing has shown this to be an async like issue. There are a few things which appear to be able to cause a glitch all have to do with frame rate. If the processor is busy it may miss its deadline. This can happen due to large amount of processing, cache disruption, IO/Memmory operations, sleeping, operating system stuff, etc. The other issue that appears to cause an issue is IO bandwidth on the networking but less so. New frames are known to cause the issue however if you play with the numbers you can usually get it mostly stable. Testing is still ongoing.

S2 Sender Card was tested with MBI5153 based panels using the same receiver card and configuration. No issues were detected with the sender card. Packet captures show that the sender card fully saturates the 1G link. A slightly different protocol was used and some of these changes were evaluated. However the result still remains. It is believe this is due to some kind of timing property which user space Linux is not able to achieve. Honestly I doubt there is a section of the kernel that is capable of this due to overhead.

A hardware module could be build which replaces the S2 sender card, however this would be somewhat costly and time consuming. Not likely to be very popular so in many ways it just does not make sense. Overall using receiver card baring firmware update for PWM/MM panels is not recommended unless doing video wall. To use S2 sender card with Pi it is recommended you use a Pi via HDMI to S2's DVI connector and create graphics program.

A working version of this hardware module is available in [LED_Matrix_PIC32MZ](https://github.com/daveythacher/LED_Matrix_PIC32MZ). This works as a sync sender card. Effort was made to control the timing of the packets so that it would not cause issues. These level of control is much harder, if even possible, in this code base.

It is potentially possible to replace the LED_Matrix_PIC32MZ logic with a BeagleBone Black's processor, AM335x. This may work without needing an external gigabit Ethernet switch. This would end up being cheaper than Raspberry Pi solution more than likely. Note the AM335x would still need to be external server in POE use cases. AM335x's RGMII would need to be used for the LED panels, while another Ethernet would be needed for application logic. This could be on USB. Coding this correctly is more involved.

## About
This logic works off shared memory map created by daemon. This enables other languages on the system to use the logic without inheriting/requiring super user. Shared memory map is creating in /tmp and is assumed to be a RAM disk or something like it. (This may not be desirable for every use case.)

Shared memory was choosen to avoid overhead of other methods. There is a command interface built into the first four bytes of memory. The daemon will poll this periodically to activate Matrix functions. Currently calling logic should wait for the command to be reset back to zero before proceeding. Current implementation is blocking to prevent writing while updating issues. (Also due to laziness.)

Note there is no protection against race conditions and hazards on shared memory. If multiple processors grab there could be problems, however this is not expected to be very common. TCP logic currently processes connections serially one at a time to prevent this issue and possible resource usage issues. Note this is still a work in progress and very lazy.

Matrix classes do not allocate memory. Instead they send the data values to the daemon which does allocate memory via shared memory. Note this shared memory is sent to IO vectors directly. Therefore many race conditions and hazards are possible. Should use proper linux user restrictions and blocking logic to prevent adverse behavior of this. This is the benefit to this approach is that offloading removes impact of inter process communication overhead to system stability. However exceptions do apply still.

Examples of language portability is shown with trigger programs. Currently Matrix class is only implemented on Linux, inorder to use on other systems this would need to be reimplemented or use network daemon running on Linux server. Note other languages could wrap up command interface into its own class wrapper.

This code base is very straight forward, and this logic is fairly light weight. This is due to the significant amount of offloading provided by the ColorLight 5A-75B. Note configuration complexity is also passed off to application logic and ColorLight configuration software. This simplifies the code down to basically a wrapper/interface logic for higher level logic.

Therefore converting this to something not based in Linux would be straightforward, given it can generate L2 packets.

I am not planning to add support for anything other than ColorLight. There are other options and I am aware of how to do that. However I am impressed with ColorLights receivers even considering the PWM issue. I had a bad experience with Linsn. The card died for unknown reason(s), software quality was lower, software is harder to work with, and PWM issues are worse. Here is an [example](https://github.com/FalconChristmas/fpp/blob/master/src/channeloutput/Linsn-RV9.cpp) that roughly matches my understanding of how it would work.

## License
This project is licensed under standard GPL 3. A copy is available [here](https://github.com/daveythacher/LED_Matrix/blob/main/LICENSE), for more details about terms and conditions that apply.

## Configuration
See [Configuration](https://github.com/daveythacher/LED_Matrix/blob/main/Configuration.md)

## Building
Daemon:
```bash
cd daemon
g++ -O3 PIC32MZ_NetCard.cpp Linux_NetCard.cpp main.cpp network.cpp -o Matrix -lpthread -lusb-1.0
```
Demo:
```bash
g++ -O3 C++/Matrix.cpp demo.cpp -o demo
```
Using multiple channels via Frame (C++):
```bash
g++ test5.cpp C++/Frame.cpp C++/Matrix.cpp -o test5 -lpthread
```
Trigger (C++):
```bash
cd trigger
g++ -O3 trigger.cpp -o trigger
```
Trigger Remote (C):
```bash
cd trigger
gcc -O3 trigger_remote.c -o trigger_remote
```
Trigger Remote (C++):
```bash
cd trigger
gcc -O3 trigger_remote.cpp ../C++/Matrix.cpp -o trigger_remote
```

## Running
Daemon:
```bash
cd daemon
sudo ./Matrix config.txt
```
Demo:
```bash
sudo ./demo -D 0
```
Trigger (C++):
```bash
./trigger
```
Trigger (Groovy):
```bash
./trigger/trigger.groovy
```
Trigger (Python):
```bash
python3 trigger.py
```
Trigger Remote (C):
```bash
./trigger_remote 127.0.0.1
```
Trigger Remote (Groovy):
```bash
groovy trigger/trigger_remote.groovy
```

## Files
The base of this repo is in Matrix.h and Matrix.cpp in daemon folder. If looking to see how it works or port this to another implementation/platform look there.

For the daemon look at main.cpp (trigger protocol) and network.cpp (TCP protocol). This logic uses Matrix.h and Matrix.cpp, which requires super user priviledge.

For the demo look at demo.cpp. This logic uses Matrix.h and Matrix.cpp in C++ folder, which does not require super user priviledge. Note it is recommended to generate logic against daemon where possible.

The groovy folder contains a Groovy Matrix class which supports both shared memory and TCP socket to daemon. A Java version of this would be easy to make. Later I may add some graphics and possibly mapping logic in Groovy here.

The C++ folder contains a C++ Matrix class which supports both shared memory and TCP socket to dameon. Later I may add some graphics and possibly mapping logic in C++ here.

For the trigger programs look at the respective implementation. Note these only work with daemon. These could be rewritten to support define a complete class which scretely ties into daemon locally or remotely. Note the performance impact of this is unknown.

## Mapping
Multiplex mapping is handled by the receiver card, however pixel mapping is handled by application logic. Currently most of the trigger programs do not bother with this. A little piece exists in the C++ and Groovy Matrix classes based on daemon, via the map_pixel virtual method. Note that is only used in set_pixel not in set_pixel_raw. (Raw is for internal use.) However creating a derived class for specific mapping should be very straight forward. This may want to be revised at some point.

This is kind of important for CC based panels which need a lot of serial bandwdith for high quality and refresh. One simple way to get this is to use multiple IO connectors provided by the receiver card. This causes the panels to be mapped vertically inside of horizontally, which is easily correct with a couple lines of code.

Other applications also likely exist which may want this. Nice thing here is you are free to use as much processing power as you want. You can run this on a computer if you need more. So you can be pretty lazy potentially. The receiver card will if nothing else continue to show the last frame it got without any issues.

## Graphics
There are several APIs for graphics processing in Java, QT, etc. Then there is OpenGL which is usually available. Granted this requires Linux and potentially higher end hardware on the server. However this may be easier/enable higher end processing function. This should also enable tighter integration functions with more resource consuming loads like 3D graphics, video, fancy web servers, etc. (Perhaps you want to use two NICs or VLAN to prevent the multicast.)

## Comparison
Trying to make a 16x128 RGB LED Matrix on PoE is possible using this approach. However other approaches are possible. Note these comparisons are only correct for this specific usage. There is a lot lurking below the covers here too. Multiple security and other features are available to the receiver card solution. However it does lack some standalone function such as scrolling without server.

### Raspberry Pi
The Raspberry Pi supports integrating a server into the box enabling lots of features. The Raspberry Pi saves around 13 to 23 dollars. The Raspberry Pi 2+ has improved stability but suffers higher power consumption. Matrix stability is a concern using the Raspberry Pi without special consideration compared to receiver card. Matrix quality is also a concern using the Raspberry Pi RGB Matrix library, however customizations may be possible to work around this. Quality issues result in reductions of color depth and refresh compared to that of receiver card. The security model for the Raspberry Pi is also a concern which increases risk for power and stability issues outside mitigation control. Note this does not really apply to the receiver card as the server is external.

More than likely all of these issues could be worked around for the most part. However this requires a decent level of understanding of LED Matrixes, Raspberry Pi, Linux, the RGB LED Matrix library, etc. Should the system be deadlocked the display will noticably suffer issues. Granted this is not expected to occur very often if at all depending on usage.

This usage even with the S-PWM update would not be able to use 11 bits for 16x128 at 30 FPS. There is not enough bandwidth at 15.6MHz to support this. The reason the receiver card can is due to the number of IO connectors. This allows each of the four 16x32 LED Matrixes to be driven in parallel. The Raspberry Pi without compute module is not capable of this. Interfacing with a FPGA or MCU is another option which is possibly more expensive compared to receiver card.

### Microcontroller
A microcontroller could be used to integrate a server into the box enabling some features available to the Raspberry Pi. This solution is generally more expensive than the receiver card. Stability and quality are not likely an issue for a microcontroller. However multiple microcontrollers are likely required or a stronger microcontroller(s) are required. Power consumption is not expected to be a huge issue for microcontroller(s). The primary benefit to a microcontroller is its security model compared to the Raspberry Pi. Note this does not really apply to the receiver card as the server is external.

In this use case there are only a few MCUs which could cleanly drive a parallel port fast enough. These are significantly more expensive. Likely would require the use of multiple microcontrollers or a FPGA to handle this. Possibly adding even more cost compared to receiver card. Exception to this is RP2040.

### PWM/MM LED Panels
It is possible to increase the number of pixels with different LED panels. However this comes with a few draw backs. 

There is a limit to the number of pixels which can be stored on the receiver card or microcontroller. There is a limit to the number of pixels for a given quality and refresh which can be supported on a microcontroller or Raspberry Pi. There is a limit to the amount of IO connector bandwidth and connectors for receiver card, microcontroller and Raspberry Pi. 

There is a limit to the amount of power which can supplied to the LED Panels. The easy way to increase pixels without changing power consumption is with higher multiplex panels. However this reduces the brightness of the panels. With some LED panels they use LED drivers which enable lowering the power consumption with software gain control rather than having to change the current limiting resistor manually.

PWM/MM Panels support longer chains without affecting quality and refresh due to improved serial protocol. These require special support which is not currently available for most micrcontrollers and Raspberry Pi. Support is available for receiver card as long as there is enough RAM/IO.

### PoE 802.3AF vs 802.3AT
802.3AF enables universal PoE support up to 100 meters. However limits the amount of power to a total of 12.95 Watts, but not all of this power is able to be used by the LEDs. Only around 8.2 Watts of power is available. Note this requires special tricks to make this possible. You cannot use 5 Volts. However does enable around 10mA per RGB pixel, assuming 1:8 multiplexing.

Note up to 2.72 Watts of the 12.95 is lost to internal conversion. Meaning an effeciency of around 79 percent is assumed. This leaves only 2 Watts for the control logic like the Receiver card. This can be difficult with the Raspberry Pi without special planning. However should not be a problem with some, if not all, receiver cards and microcontroller(s). 

Note stealing additional power from LEDs is also possible which may result in a trivial amount of brightness reduction. However this is not recommended. Even less so is possibly limiting the number of RGB pixels on at a time to limit LED power consumption.

### Color Depth
Most RGB images use 24 bit color depth meaning there is 16.7 million colors. However supporting this on LED matrixes is complicated issue. Many LEDs have a max forward current of 20mA but can light around a few microamps. Many panels white color balance at 5 volts with a R/G/B current ratio of around 16/9/7mA. Meaning the max current steps possible is around 7000. Multiplexing however can lower this to 218 for 32 scan, which means we are down to around 8 bits of color depth per color.

However LED brightness is directly correlated to current. However LED color perception is not directly correlated to current. A mapping function must be used which increases the grandularity of current division to achieve higher accuracy or control over the color perception. This means that for a 8 bit color depth input per color you will need to use possible 11 bits of color depth per color.

Naturally there could be slight or significant reduction of color depth if the panels is not capable of supporting the full color depth required. For example the green LED may not have enough color depth to support full color with the mapping algorithm. However this is not fully understood. For most many uses cases this may be hard to notice.

Note mileage will vary. Some panels are built with lower multiplex ratios and some LEDs have better color depth.

#### PWM
Most commonly dividing the current is done with BCM. There are two major types of PWM, hardware and software. For hardware PWM the LED drivers like have standard PWM generators. While for software PWM there is likely indirect control via shift registers using binary coded modulation (BCM) or sometimes called bit angle modulation (BAM).

For refresh enhancement there is also two major types, traditional PWM and scrabled PWM (S-PWM). Traditional PWM will spend the entire PWM period per row. While S-PWM will divide the PWM period into segments and only spend one segment per row. This the overall perceptive refresh is increased with S-PWM. S-PWM is believed to be used in receiver cards for non-PWM LED panels. Memory mapped (MM) LED drivers commonly use S-PWM along with some PWM LED drivers.

PWM generators are usually clock via a grayscale clock. This clock derives the refresh rate. The clock rate is the number of PWM steps times the multiplex scan and frame rate. S-PWM divides the PWM steps so it increases the refresh rate. Note for traditional PWM the frame rate may be the refresh rate.

##### S-PWM on Pi and MCUs
This believed to be challenging on the Pi due to scheduling and arbitration issues which can emergy. Scheduling and arbitration issues examples: OS schdeuling due to tick rate (4ms), DDR memory inteferace arbriration, DDR burst delay, Atomic GPIO port arbitration, system bus arbitration, interrupts, kernel events, etc. BCM using tradditional PWM tend to work out a lot of these issues assuming proper configuration. Note CPU is used over DMA due to DDR as L1 cache is SRAM and increases DDR efficiency.

It is believe this is possible on some or many MCUs due to more executive control, deteriminism, IO resources, SRAM, etc. However to date I am not aware of this existing. Most real time operating systems can achieve better tick rates. MCUs tend to have options like paced hardware modules, FIFOs, DMA based on SRAM, more control of priority/arbitration, etc. However suffer in processing capabity, cost and memory density.

Note this may increase overhead and increase configuration complexity. It should be possible to integrate this with BCM but may be a little weird.

### LED Rail Voltage
To lower the power consumption of the LED panels there is two methods, lower the current and/or lower the voltage. If you lower the voltage to the panels you will lose white color balance as the red LED current drivers will likely remain operational longer than the green and blue LEDs. This is due to the fact that LEDs are diodes which means voltage is not directly correlated with current.

In order to get this balance back you need to rebalance the current per color in roughly the same ratios. Each LEDs has its own forward voltage threshold however most of the time the red LEDs is much lower than green and especially blue. The high side PMOS drivers need a small amount of voltage drop for stable operation. As does the low side constant current drivers. Therefore if you lower the 5 volts rail to 3.3 volts, assuming the logic components can tolerate this, you will need to rebase the current for stable operation. This will vary from panel to panel.

There are two major ways to do this. Most panels use current amplifiers internally. The amplifiers uses a reference current which is then multipled by fixed multiplier to change the current. This reference current is set using a known voltage and resistor. In most panels you need to change the panels. The other approach is to use the software gain control which allows changing the fixed gain multiplier, therefore a resistor change may not be required.

Usually you will need to track down the LED driver datasheet to get things like the multipler, reference voltage, stable current thresholds, etc. After getting the resistances from the SMD resistors you can work back the ratios and resistances. From here you just lower the currents till stable or to the desired power operating level. 

Thus for lowering the voltage you must lower the current most of the time. You will be able to use the panel at higher voltage, but in order to get the lower power draw or high efficiency you will need to lower the voltage also. This is likely required for PoE operation.

## VLAN / Switch
It is recommended to use VLAN to contain packet transmission. While not required it may be desired for security or functional purposes. The reason for the recommendation is prevent flooding, which is possible since the L2 switch may never be able to associate the packet to a single port. This may sound like a flaw but it is potentially more stable. These are likely designed to work on dedicated L2 segements anyhow for stability concerns. VLANs are an alternative to physical segementation.

If looking to restrict access to the display, L2 segement issolation can prevent unathorized access to display and network traffic. Note if using VLANs, configuration of network and networking devices (such as smart switches) can represent possible concerns for security. When in doubt use physical issolation.

Receiver cards are not known to support addressing outside of physical link. However it is possible to use virtual links to work around this. This allows multiple displays to be partitioned into groups or addressed individually. Port mirroring would enable duplication of messages to multiple displays/groups. This can reduce the complexity of software and network bandwidth significantly. 

Cost savings can be high here since PoE or standard switches are likely required anyhow. However multiple physical segements via multiple NIC cards is also possible. This may represent as safer and possibly low cost option in some cases.

Example of small PoE Switch for a one to four displays is provided below. This product family has higher port configurations available for larger setups.

https://www.amazon.com/TP-Link-Lifetime-Protection-Aggregation-TL-SG105PE/dp/B08D73YD5S

## Compared to Internal Pi/MCU
You can save 13-35 dollars depending how much the receiver card costs (16-38) dollars. However this is only really useful for one offs. Installing a 16x128 display into multiple rooms driven from single server over switch is cheaper with this approach. Pi/MCU likely would require MM panels which are generally larger and more expensive. This could work however the visibly due to shape and brightness may be less.

There could be security concerns with internal Pi/MCU depending on design.
