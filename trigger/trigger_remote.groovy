#!/usr/bin/env groovy

/* 
 * File:   trigger_remote.groovy
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 25, 2021
 */

import groovy.Matrix;
import groovy.Matrix_RGB_t;

// Using Matrix class
def m = new Matrix("127.0.0.1")
def p = new Matrix_RGB_t((byte) 255, (byte) 255, (byte) 255)
m.clear()
m.set_pixel(0, 0, p)
m.send_frame()
sleep(3000)
m.send_frame(13)
rows = m.get_rows()
cols = m.get_columns()
m.fill(new Matrix_RGB_t((byte) 255, (byte) 255, (byte) 255))
m.send_frame()
