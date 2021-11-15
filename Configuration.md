# Configuration
## Receiver Card
### Configuration
### Mapping
### Color Depth
## Daemon
The daemon needs to be configured with a configuration file. Multiple channels are supported now. Each channel requires eight parameters.

Note two threads are used per channel. 

### Configuration File
The configuration file requires four lines per channel. Note there is no blank lines in configuration file, therefore channels start one after the other. The configuration file logic is very simple and brittle.

The first line contains the channel number (0 to 255) followed by TCP port (0 to 65535) separated by a single space. The second line contains the networking interface name as a string as seen by ifconfig. Note multiple channels can use the same or different interface name. The third line contains the number of rows (1 to 1024) used by the receiver card followed by the number of columns (1 to 1024) used by the receiver card separated by a single space. Note these must match the configuration settings of the receiver card. The forth line contains the VLAN enable flag (1 for VLAN and 0 for non-VLAN) followed by the VLAN id (0 - 4095) separated by a single space. Note the id must always be provided, however is only used if enabled.

Note channels should never share an interface without a VLAN. Channels should not share a VLAN ever, unless port mirroring is used.

Note wrapper classes will not work without daemon running. Daemon needs to be ran with super user rights. The application logic/wrapper does not require this, and this is the point of the daemon. (To restrict the priviledge scope.) The point of users, kernel, etc. is to not have things running with full control/access. This does create some overhead which in some applications is undesireable, raw source is available for them. In most case with this type of logic there will be little to no impact. The intensive functions which would normally be corrupted by this are relocated off device.

### PIC32MZ NetCard over USB
The daemon has been modified to support using PIC32MZ NetCard. The same configuration file is used, using the same structure. To add an entry for PIC32MZ NetCard over USB, set the networking interface name to USB, just those three characters. Note VLAN settings are ignored, as these are not supported by the PIC32MZ NetCard. Everything else works the exact same.

Note the set_brightness method does not work on PIC32MZ NetCard's current USB protocol and firmware. This could be added later on.

### PIC32MZ NetCard Firmware Resolution
The PIC32MZ NetCard is hard coded currently to support a max chain length of 256. ColorLight 5A-75B supports up to 256x256 and ColorLight 5A-75E supports up to 512x256. The ColorLight 5A-75B has eight connectors while the ColorLight 5A-75E has sixteen connectors, meaning the max panel that can be connected to each connector if using all connectors is 64x128. The normal firmware limits the column size to 1024, and I think the PWM firmware does also. However this is further reduced by PIC32MZ NetCard.

This makes this a little more complicated. However this means that you should configure the receiver card to match the configuration of PIC32MZ NetCard firmware. At that point the workflow is exactly the same. Meaning configuration file rows and columns should match the receiver cards value.

Note the firmware can be updated to increase the row size at some point, see Linux_NetCard's implementation in the daemon for details. Note PIC32MZ_NetCard's implementation maps pixels based on 256 column limits and this is hard coded.

## Wrapper
To my knowledge there is no way of converting the panel/cabinet pixel configuration to the desired pixel configuration. This is easily corrected with a mapping function. Currently this requires creating a derived wrapper Matrix class. This requires coding.

Once this is done you must create the derived wrapper Matrix object with the desired size. From this point forward you will be able to use set_pixel functions as if you were working with that size. This allows the programming to follow the actual application location rather than the receiver card location which could be different due to implementation constraints or optimizations.

By default this project is configured for 64x32 at the receiver card. Meaning it supports 4 parallel chains of one 16x32 LED panel. The desired application is 16x128 to create a scroll/message board, however the performance and qaulity of longer chains is poor. Therefore it uses four independent chains to improve the performance by giving each panel its own serial bandwidth. By default the map_pixel function and wrapper class size is configured for 16x128. It will convert the 16x128 into 64x32 using blocks of 16x32.
