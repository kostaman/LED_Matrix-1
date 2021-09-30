/* 
 * File:  Matrix.groovy
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 25, 2021
 */
 
package groovy
import groovy.Matrix_RGB_t;
import java.nio.*;
import java.nio.channels.FileChannel;

class Matrix {
	Matrix() {
		map = new RandomAccessFile("/tmp/LED_Matrix.mem", "rw").getChannel().map(FileChannel.MapMode.READ_WRITE, 0, new File("/tmp/LED_Matrix.mem").length())
		map.put(0, (Byte) 3)
		while (map.get(0));
		rows = (map.get(2) << 8) + map.get(3)
		map.put(0, (Byte) 4)
		while (map.get(0));
		cols = (map.get(2) << 8) + map.get(3)
		isNet = false
	}
		
	Matrix(String address) {
		addr = InetAddress.getByName(address)
		isNet = true
		def s = new Socket(addr, 8080)
		s.withStreams { istream, ostream ->
			def data = [0x21, 0x20, 0x20, 0x09, 2, 0, 0, 0 ] as byte[]
			ostream.write(data, 0, data.size())
			ostream.flush()
			while(istream.available() < 4);
			istream.read(data, 0, 4)
			cols = data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0]
			while(istream.available() < 4);
			istream.read(data, 0, 4)
			rows = data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0]
		}
		s.close()
	}
		
	def get_rows() {
		return 16
	}
	
	def get_columns() { 
		return 128
	}
		
	def send_frame() {
		if (!isNet) {
			map.put(0, (byte) 1)
			while(map.get(0));
		}
		else {
			def s = new Socket(addr, 8080)
			s.withStreams { istream, ostream ->
				def data = [0x21, 0x20, 0x20, 0x09, 1, 0, 0, 0 ] as byte[]
				ostream.write(data, 0, data.size())
				ostream.flush()
			}
			s.close()
		}
	}
		
	def send_frame(int vlan_id) {
		if (!isNet) {
			map.put(2, (byte) vlan_id)
			map.put(1, (byte) (vlan_id >> 8))
			map.put(0, (byte) 2)
			while(map.get(0));
		}
		else {
			def s = new Socket(addr, 8080)
			s.withStreams { istream, ostream ->
				def data = [0x21, 0x20, 0x20, 0x09, 0, 2, 0, 0, vlan_id, vlan_id >> 8] as byte[]
				ostream.write(data, 0, data.size())
				ostream.flush()
			}
			s.close()
		}
	}
	
	def set_pixel(int x, int y, Matrix_RGB_t pixel) {
		x %= get_columns()
		y %= get_rows()
		def c = map_pixel(x, y)
		set_pixel_raw(c.x, c.y, pixel)
	}
	
	def set_pixel(int x, int y, Matrix_RGB_t[] pixels) {	// TODO: Test
		if (!isNet) {
			def v = y * get_columns() + x as int
			for (Matrix_RGB_t pixel : pixels) {
				v %= rows * cols
				x = v % get_columns()
				y = v / get_columns()
				set_pixel(x, y, pixel)
				v++
			}
		}
		else
			throw new Exception("Setting multiple pixels over TCP/network is only supported using set_pixel_raw")
	}
	
	def fill(Matrix_RGB_t pixel) {
		if (!isNet) {
			for (int y = 0; y < rows; y++) {
				for (int x = 0; x < cols; x++) {
					map.put(y * cols + x + 4, pixel.red)
					map.put(y * cols + x + 5, pixel.green)
					map.put(y * cols + x + 6, pixel.blue)
				}
			}
		}
		else {
			def s = new Socket(addr, 8080)
			s.withStreams { istream, ostream ->
				def data = [0x21, 0x20, 0x20, 0x09, 5, 3, 0, 0, pixel.red, pixel.green, pixel.blue] as byte[]
				ostream.write(data, 0, data.size())
				ostream.flush()
			}
			s.close()
		}
	}
	
	def clear() {
		fill(new Matrix_RGB_t((byte) 0, (byte) 0, (byte) 0))
	}
	
	def set_brightness(byte brightness) {
		if (!isNet) {
			map.put(1, brightness)
			map.put(0, (byte) 5)
			while(map.get(0));
		}
		else {
			def s = new Socket(addr, 8080)
			s.withStreams { istream, ostream ->
				def data = [0x21, 0x20, 0x20, 0x09, 6, 1, 0, 0, brightness] as byte[]
				ostream.write(data, 0, data.size())
				ostream.flush()
			}
			s.close()
		}
	}
	
	protected def map_pixel(int x, int y) {
		def c = new pixel_cord()
		c.x = x % cols
		c.y = (x / cols * 16) + (y % 16)
		return c
	}
	
	protected def set_pixel_raw(int x, int y, Matrix_RGB_t pixel) {
		if (!isNet) {
			x %= cols
			y %= rows
			map.put(y * cols + x + 4, pixel.red)
			map.put(y * cols + x + 5, pixel.green)
			map.put(y * cols + x + 6, pixel.blue)
		}
		else {
			def v = y * cols + x as int
			v %= rows * cols
			def s = new Socket(addr, 8080)
			s.withStreams { istream, ostream ->
				def data = [0x21, 0x20, 0x20, 0x09, 3, 7, 0, 0, v, v >> 8, v >> 16, v >> 24, pixel.red, pixel.green, pixel.blue] as byte[]
				ostream.write(data, 0, data.size())
				ostream.flush()
			}
			s.close()
		}
	}
	
	protected def set_pixel_raw(int x, int y, Matrix_RGB_t[] pixels) {	// TODO: Test
		def v = y * cols + x as int
		v %= rows * cols
		if (!isNet) {
			for (Matrix_RGB_t pixel : pixels) {
				x = v % get_columns()
				y = v / get_columns()
				set_pixel_raw(x, y, pixel)
				v++
				v %= rows * cols
			}
		}
		else {
			def s = new Socket(addr, 8080)
			// TODO: Fix bug pixel.size() larger than 83
			if (pixels.size() > 83)
				throw new Exception("Currently cannot pass more than 83 pixels at a time to set_pixel_raw over TCP/network")
			s.withStreams { istream, ostream ->
				def data = [0x21, 0x20, 0x20, 0x09, 3, 4 + (pixels.size() * 3), 0, 0, v, v >> 8, v >> 16, v >> 24] as byte[]
				ostream.write(data, 0, data.size())
				ostream.flush()
				for (Matrix_RGB_t pixel : pixels) {
					data[0] = pixel.red
					data[1] = pixel.green
					data[2] = pixel.blue
					ostream.write(data, 0, 3)
					ostream.flush()
				}
			}
			s.close()
		
		}
	}
	
	protected MappedByteBuffer map
	protected InetAddress addr
	protected int rows
	protected int cols
	protected boolean isNet
	
	protected class pixel_cord {
		pixel_cord() {
			x = 0
			y = 0
		}
		
		int x
		int y
	}
}
