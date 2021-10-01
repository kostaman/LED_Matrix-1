# Configuration
## Receiver Card
### Configuration
### Mapping
### Color Depth
## Daemon
The daemon needs to be configured with three inputs. Note the TCP port is hard coded to 8080. The first is the name of the Ethernet network interface as reported by ifconfig to be used by the daemon. Next is the number of rows used by the receiver card. Followed by the number of columns used by the receiver card. These must match the configuration settings of the receiver card.

## Wrapper
To my knowledge there is no way of converting the panel/cabinet pixel configuration to the desired pixel configuration. This is easily corrected with a mapping function. Currently this requires creating a derived wrapper Matrix class. This requires coding.

Once this is done you must create the derived wrapper Matrix object with the desired size. From this point forward you will be able to use set_pixel functions as if you were working with that size. This allows the programming to follow the actual application location rather than the receiver card location which could be different due to implementation constraints or optimizations.

By default this project is configured for 64x32 at the receiver card. Meaning it supports 4 parallel chains of one 16x32 LED panel. The desired application is 16x128 to create a scroll/message board, however the performance and qaulity of longer chains is poor. Therefore it uses four independent chains to improve the performance by giving each panel its own serial bandwidth. By default the map_pixel function and wrapper class size is configured for 16x128. It will convert the 16x128 into 64x32 using blocks of 16x32.
