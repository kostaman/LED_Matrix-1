#!/usr/bin/env groovy

/* 
 * File:   trigger.groovy
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 19, 2021
 */

import java.nio.channels.FileChannel;

// Map shared memory buffer
def map = new RandomAccessFile("/tmp/LED_Matrix.mem", "rw").getChannel().map(FileChannel.MapMode.READ_WRITE, 0, new File("/tmp/LED_Matrix.mem").length())

// Set pixel
map.put(4, (Byte) 255)

// Trigger send_frame
map.put(0, (Byte) 1)

// Wait for trigger to finish
while (map.get(0))
	println("Waiting")

// Trigger send_frame with VLAN 13
map.put(2, (Byte) 13)
map.put(0, (Byte) 2)

// Get the number of Columns
map.put(0, (Byte) 3)
while (map.get(0));
def rows = (map.get(2) << 8) + map.get(3)

// Get the number of Rows
map.put(0, (Byte) 4)
while (map.get(0));
def cols = (map.get(2) << 8) + map.get(3)

// Fill display all white
map.put(1, (Byte) 255);
map.put(2, (Byte) 255);
map.put(3, (Byte) 255);
map.put(0, (Byte) 6);
