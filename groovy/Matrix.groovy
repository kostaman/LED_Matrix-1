package groovy
import java.nio.*;
import java.nio.channels.FileChannel;

class Matrix_RGB_t {
	Matrix_RGB_t(byte r, byte g, byte b) {
		red = r
		green = g
		blue = b
	}

	byte red
	byte green
	byte blue
}

class Matrix_mmem {
	Matrix_mmem() {
		map = new RandomAccessFile("/tmp/LED_Matrix.mem", "rw").getChannel().map(FileChannel.MapMode.READ_WRITE, 0, new File("/tmp/LED_Matrix.mem").length())
		map.put(0, (Byte) 3)
		while (map.get(0));
		rows = (map.get(2) << 8) + map.get(3)
		map.put(0, (Byte) 4)
		while (map.get(0));
		cols = (map.get(2) << 8) + map.get(3)
	}
		
	/*Matrix(InetAddress addr) {
		
	}*/
		
	def get_rows() {
		return 16
	}
	
	def get_columns() { 
		return 128
	}
		
	def send_frame() {
		map.put(0, (byte) 1)
		while(map.get(0));
	}
		
	def send_frame(int vlan_id) {
		map.put(2, (byte) id & 0xFF)
		map.put(1, (byte) id >> 8 & 0xFF)
		map.put(0, (byte) 2)
		while(map.get(0));
	}
	
	def set_pixel(int x, int y, Matrix_RGB_t pixel) {
		def c = map_pixel(x, y)
		set_pixel_raw(c.x, c.y, pixel)
	}
	
	def set_pixel_raw(int x, int y, Matrix_RGB_t pixel) {
		map.put(y * cols + x + 4, pixel.red)
		map.put(y * cols + x + 5, pixel.green)
		map.put(y * cols + x + 6, pixel.blue)
	}
	
	def fill(Matrix_RGB_t pixel) {
		for (int y = 0; y < rows; y++) {
			for (int x = 0; x < cols; x++) {
				map.put(y * cols + x + 4, pixel.red)
				map.put(y * cols + x + 5, pixel.green)
				map.put(y * cols + x + 6, pixel.blue)
			}
		}
	}
	
	def clear() {
		fill(Matrix_RGB_t())
	}
	
	def set_brightness(byte brightness) {
		map.put(1, brightness)
		map.put(0, (byte) 5)
		while(map.get(0));
	}
	
	protected def map_pixel(int x, int y) {
		pixel_cord c
		c.x = x % cols
		c.y = (x / cols * 16) + (y % 16)
		return c
	}
	
	protected MappedByteBuffer map
	protected int rows
	protected int cols
	
	protected class pixel_cord {
		int x
		int y
	}
}
