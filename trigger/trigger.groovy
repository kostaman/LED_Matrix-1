#!/usr/bin/env groovy

/* 
 * File:   trigger.groovy
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 19, 2021
 */

import groovy.Matrix;
import groovy.Matrix_RGB_t;

import java.nio.channels.FileChannel;

// Map shared memory buffer
def map = new RandomAccessFile("/tmp/LED_Matrix-0.mem", "rw").getChannel().map(FileChannel.MapMode.READ_WRITE, 0, new File("/tmp/LED_Matrix-0.mem").length())

// Set pixel
map.put(4, (Byte) 255)

// Trigger send_frame
map.put(0, (Byte) 1)

// Wait for trigger to finish
while (map.get(0));

// Trigger send_frame with VLAN 13
/*map.put(2, (Byte) 13)
map.put(0, (Byte) 2)
while (map.get(0));*/

// Get the number of Rows
map.put(0, (Byte) 3)
while (map.get(0));
def rows = (map.get(2) << 8) + map.get(3)

// Get the number of Columns
map.put(0, (Byte) 4)
while (map.get(0));
def cols = (map.get(2) << 8) + map.get(3)

// Fill buffer all white
map.put(1, (Byte) 255);
map.put(2, (Byte) 255);
map.put(3, (Byte) 255);
map.put(0, (Byte) 6);
while (map.get(0));

// Trigger send_frame
map.put(0, (Byte) 1)
while (map.get(0));


// Using Matrix class
def m = new Matrix()
def p = new Matrix_RGB_t((byte) 255, (byte) 255, (byte) 255)
m.set_pixel(0, 0, p)
m.send_frame()
//m.send_frame(13)
rows = m.get_rows()
cols = m.get_columns()
m.fill(new Matrix_RGB_t((byte) 255, (byte) 255, (byte) 255))
m.send_frame()
