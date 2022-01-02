# Configuration
## Limits
There are limits which restrict the amount of color depth possible in the display. Color depth can be limited by size, refresh/frame rate, power supply response time, and the LEDs themselves. Below are three main categories.

### LED Current
LEDs are diodes and thus have an IV plot which means that for any given forward current there will be a different voltage. The relationship for is non-linear and usually somewhat exponential. LEDs however will not light below a certain point. This point devices the max color depth possible. 

Note the IV plot allows reworking the panels to use less power beyond simply lowering the current consumption of the LEDs. Since LEDs need less forward voltage when they use less forward current. This allows optimization of the LED power rail to lower the overall consumption. This does however generally lower the color depth possible.

To find the limit you simply forward current of the lowest current LED color, usually Blue. Then find the lowest current the that LED will light at. Take the normal forward current of the that LED and divide it by the min light current. To convert this to PWM bits take log2 of the number.

Note thus far this number assumes no multiplexing and therefore you need to subtract the log2 of the multiplex ratio. This will be the max current division possible, however using a few extra bits may be desired for certain conversion algorithms.

### Serial Bandwidth
Serial bandwidth can play a siginificant role depending on the control algorithm used. Below are majory categories.

#### non-PWM Drivers:
These drivers are the most basic of LED drivers and use software PWM and/or BCM. These have some advantages but are generally less effiecent. Below are two major categories for software PWM algorithms.

##### Traditional PWM - 
The issue with this approach is the any refresh consumes significant amounts of serial bandwidth. This lowers the max color depth against chain length siginificantly. Generally speaking this will likely be a siginficant factor for just about any display beyond a certain point. Log2 of max serial clock divided by refresh, multiplex and columns gives color depth.

Note longer chains will likely need to consider the serial clock speed in order to keep the chain stable. This likely involves stepping back the clock rate which will lower the serial bandwidth available. Refresh generally should be around 100Hz min.

##### S-PWM - 
This approach solves one of the issues with traditional PWM, however can be a little harder to implement. Full version of this may increase memory consumption, increase processing power requirements and require higher levels of determinism. However this enables serial bandwidth to no longer repeat for every refresh. Instead the refresh becomes the actual FPS needed. This allows the refresh to drop to one. Thus providing a decent acceleration.

Note this works by dividing the PWM period into multiple sections. Each multiplex scan only shifts out one section per pass, thus improving the perceived refresh. When used with a decent amount of color depth it may be recommended to normalize the on time accross the total period rather than doing it on the very first section. To normalize you generally can use something like a binary tree algorithm. Note this is more resource intensive approach compared to just dividing up traditional PWM which is fairly low resource consumption approach.

#### PWM Drivers:
These drivers are optimized version of non-PWM drivers. These improve the performance of increases color depth against chain length. However they still struggle with high multiplex applications due to a lack of memory. They work by removing the software PWM and replace it with hardware PWM which removes the excess serial bandwidth consumption. These generally favor high PWM bits and low multiplex just like the LED current reflects, however this is not completely true.

Note these generally implement traditional PWM. Again due to the problem with multiplexing there can be a lot of waste and this can result in lower refreshes. However there are some versions which support S-PWM. There are cases where this is desireable and can lower processing power requirements.

Chain length is likely limited here by color depth. Color depth is not likely a significant limitation due to PWM drivers. These are slightly more expensive compared to non-PWM drivers.

#### PWM Drivers with Memory:
These drivers are optimized version of PWM drivers. These improve the performance of multiplexing against chain length as they store all information for the whole frame. A state machine is used inside to assit in the multiplexing thus removing excess serial bandwidth consumption. These generally work with S-PWM and therefore due not waste bandwidth on refresh/multiplexing also. However due to memory the FPS can be any number above zero instead of one or higher. 

Chain length can be quite large. Color depth is not likely a significant limitation due to PWM drivers with memory. These are slightly more expensive compared to PWM drivers.

### Power Supply Response Time
The amount of time it takes to feed the LEDs power can be the difference between the LED lighting and not lightling. Depending on the LED driver the response times required can make the LED drivers deceptive.

Generally speaking the max color depth is log2 of the inverse of the response time divided by multiplex and FPS. Remember FPS is refresh in traditional PWM. This can be a significant limiting factor however getting quality power supplies is not hard. Note wiring and other factors also play a role.

 Below are the two major categories of LED drivers, each has their own requirements based on how the implement color depth.

#### non-PWM Drivers:
The power supply roughly must be able to respond as fast as the serial clock divided by the number of columns. Slower power supplies may be thinked of as low pass filters which block fast signals and shave off a little on time depending on the configured response time.

Below are two major categories for software PWM alogirthms, each has a slightly different issue created by this.

##### PWM -
PWM creates a single on time per period, assuming traditional PWM. This makes this most accurate and brightest mode for slower power supplies. If the power supply is slow to respond it will reduce some of the on time to the single edge, which could be the entire on time. The issue with this is the processing power required is fairly intensive.

##### BCM -
BCM can create multiple on times per period, assuming traditional PWM. This means it can be very inaccurate and dim for slower power supplies. If the power supply is slow it will reduce most or all of the LSBs to the point that they will not exist or contribute. The benefit to this is the processing power required is fairly small. 

#### PWM Drivers:
The power supply needs to be very fast. The problem with all that effiecency is that it increases the response time. Generally speaking the response time is the grayscale clock.

This gets a little tricky in a few cases below are the two categories.

##### Traditional PWM -
Here the issue is lowering the grayscale clock lowers the refresh as you spend more time per period. There really is no solution here. You may just wish to let the power supply clip your period and lower your color depth.

##### S-PWM -
Here the issue is you need to consider the FPS of the serial bandwidth in the grayscale clock against color depth. You have the same issue as traditional PWM where you intentionally increase the grayscale clock to increase the refresh rate.

## Receiver Card
### Configuration
### Mapping
### Color Depth

## Daemon
The daemon needs to be configured with a configuration file. Multiple channels are supported now. Each channel requires seven parameters.

Note two threads are used per channel. 

### Configuration File
The configuration file requires four lines per channel. Note there is no blank lines in configuration file, therefore channels start one after the other. The configuration file logic is very simple and brittle.

The first line contains the channel number (0 to 255) followed by TCP port (0 to 65535) separated by a single space. The second line contains the networking interface name as a string as seen by ifconfig. Note multiple channels can use the same or different interface name. The third line contains the number of rows (1 to 1024) used by the receiver card followed by the number of columns (1 to 1024) used by the receiver card separated by a single space. Note these must match the configuration settings of the receiver card. The forth line contains the VLAN enable flag (1 for VLAN and 0 for non-VLAN) followed by the VLAN id (0 - 4095) separated by a single space. Note the id must always be provided, however is only used if enabled.

Note channels should never share an interface without a VLAN. Channels should not share a VLAN ever, unless port mirroring is used.

Note wrapper classes will not work without daemon running. Daemon needs to be ran with super user rights. The application logic/wrapper does not require this, and this is the point of the daemon. (To restrict the priviledge scope.) The point of users, kernel, etc. is to not have things running with full control/access. This does create some overhead which in some applications is undesireable, raw source is available for them. In most case with this type of logic there will be little to no impact. The intensive functions which would normally be corrupted by this are relocated off device.

### PIC32MZ NetCard over USB
The daemon has been modified to support using PIC32MZ NetCard. The same configuration file is used, using the same structure. To add an entry for PIC32MZ NetCard over USB, set the networking interface name to USB, just those three characters.

### PIC32MZ NetCard Firmware Resolution
The PIC32MZ NetCard supports up to 131072 pixels. The firmware will accept rows up to 512 and columns up to 1280 as standard for the NetCards. However the pixel support of the PIC32MZ is five times less. It will convert any setting given which exceeds this automatically. It favors rows over columns. Meaning it will allocate the max rows supported then reduce the columns to the supported limit. Note this is still enough to support two ColorLight 5A-75B or a single ColorLight 5A-75E completely. However driving multiple receiver cards which do not use max resolution per card is also supported.

The daemon should continue to be configured based on the receiver card configuration. The PIC32MZ_NetCard implementation will configure the PIC32MZ firmware automatically for you. So you should be able to ignore the PIC32MZ from the configuration. Note this memory limit is not a huge deal given the Ethernet and USB bandwidth limitations.

## Wrapper
To my knowledge there is no way of converting the panel/cabinet pixel configuration to the desired pixel configuration. This is easily corrected with a mapping function. Currently this requires creating a derived wrapper Matrix class. This requires coding.

Once this is done you must create the derived wrapper Matrix object with the desired size. From this point forward you will be able to use set_pixel functions as if you were working with that size. This allows the programming to follow the actual application location rather than the receiver card location which could be different due to implementation constraints or optimizations.

By default this project is configured for 64x32 at the receiver card. Meaning it supports 4 parallel chains of one 16x32 LED panel. The desired application is 16x128 to create a scroll/message board, however the performance and qaulity of longer chains is poor. Therefore it uses four independent chains to improve the performance by giving each panel its own serial bandwidth. By default the map_pixel function and wrapper class size is configured for 16x128. It will convert the 16x128 into 64x32 using blocks of 16x32.
