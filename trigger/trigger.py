#!/usr/bin/env python3

'''
 File:   trigger.py
 Author: David Thacher
 License: GPL 3.0
 
 Created on September 19, 2021
 ''' 
 
import mmap

f = open("/tmp/LED_Matrix-0.mem", "r+b")
mm = mmap.mmap(f.fileno(), length=0, access=mmap.ACCESS_WRITE)

# Set pixel
mm.seek(4)
mm.write_byte(255)

# Trigger send_frame
mm.seek(0)
mm.write_byte(1)

# Wait for trigger to finish
while True:
	mm.seek(0)
	if mm.read_byte() == 0:
		break

# Trigger send_frame with VLAN 13
#mm.seek(2)
#mm.write_byte(13)
#mm.seek(0)
#mm.write_byte(2)	
