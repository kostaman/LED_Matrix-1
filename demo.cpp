/*
 * File: demo.cpp
 * Modifier: David Thacher
 * Data: September 21, 2021
 *
 * Original version: https://github.com/hzeller/rpi-rgb-led-matrix/blob/1362da5a2991a53f567c68c17f161731a66cdf8e/examples-api-use/demo-main.cc
 * Original License: public domain
 * 
 * Current License: GPL 3 when used with https://github.com/daveythacher/LED_Matrix, otherwise orignal license applies to this file
 *
 * Warning: This still need a lot of work.
 *
 * Modifications:
 *	9/21/21 - David Thacher: Ported to work with https://github.com/daveythacher/LED_Matrix
					Removed classes BrightnessPulseGenerator, GeneticColors, SimpleSquare, VolumeBars, ImageScroller
 *	9/22/21 - David Thacher: Ported BrightnessPulseGenerator
 *	9/24/21 - David Thacher: Ported ImageScroller, some characters are blurry in runtext16.ppm (I am suspecting issue with test file)
 *	9/30/21 - David Thacher: Switched to daemon logic. (Currently uses shared memory.)
 */

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "C++/Matrix.h"

#include <algorithm>
using std::min;
using std::max;

#define TERM_ERR  "\033[1;31m"
#define TERM_NORM "\033[0m"

class DemoRunner {
  public:
    virtual void Run() = 0;
    
  protected:
    DemoRunner(Matrix *matrix) : m(matrix) {}
    virtual ~DemoRunner() {}
    
    Matrix *m;
};

/*
 * The following are demo image generators. They all use the utility
 * class DemoRunner to generate new frames.
 */

// Simple generator that pulses through RGB and White.
class ColorPulseGenerator : public DemoRunner {
public:
  ColorPulseGenerator(Matrix *m) : DemoRunner(m) {}
  void Run() override {
    uint32_t continuum = 0;
    while (1) {
      usleep(5 * 1000);
      continuum += 1;
      continuum %= 3 * 255;
      int r = 0, g = 0, b = 0;
      if (continuum <= 255) {
        int c = continuum;
        b = 255 - c;
        r = c;
      } else if (continuum > 255 && continuum <= 511) {
        int c = continuum - 256;
        r = 255 - c;
        g = c;
      } else {
        int c = continuum - 512;
        g = 255 - c;
        b = c;
      }
      m->fill(Matrix_RGB_t(r, g, b));
      m->send_frame();
    }
  }

private:
};

// Simple generator that pulses through brightness on red, green, blue and white
class BrightnessPulseGenerator : public DemoRunner {
public:
  BrightnessPulseGenerator(Matrix *m) : DemoRunner(m) {}
  void Run() override {
    const uint8_t max_brightness = 100;
    const uint8_t c = 255;
    uint8_t count = 0;
    uint8_t x = max_brightness;

    while (1) {
      if (++x > max_brightness) {
      	 x = 0;
        count++;
      }
      m->set_brightness(x);

      switch (count % 4) {
      case 0: m->fill(Matrix_RGB_t(c, 0, 0)); break;
      case 1: m->fill(Matrix_RGB_t(0, c, 0)); break;
      case 2: m->fill(Matrix_RGB_t(0, 0, c)); break;
      case 3: m->fill(Matrix_RGB_t(c, c, c)); break;
      }
      
      m->send_frame();
      usleep(20 * 1000);
    }
  }
};

class GrayScaleBlock : public DemoRunner {
public:
  GrayScaleBlock(Matrix *m) : DemoRunner(m) {}
  void Run() override {
    const int sub_blocks = 16;
    const int width = m->get_columns();
    const int height = m->get_rows();
    const int x_step = max(1, width / sub_blocks);
    const int y_step = max(1, height / sub_blocks);
    uint8_t count = 0;
    while (1) {
      for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
          int c = sub_blocks * (y / y_step) + x / x_step;
          switch (count % 4) {
          case 0: m->set_pixel(x, y, Matrix_RGB_t(c, c, c)); break;
          case 1: m->set_pixel(x, y, Matrix_RGB_t(c, 0, 0)); break;
          case 2: m->set_pixel(x, y, Matrix_RGB_t(0, c, 0)); break;
          case 3: m->set_pixel(x, y, Matrix_RGB_t(0, 0, c)); break;
          }
        }
      }
      m->send_frame();
      count++;
      sleep(2);
    }
  }
};

// Simple class that generates a rotating block on the screen.
class RotatingBlockGenerator : public DemoRunner {
public:
  RotatingBlockGenerator(Matrix *m) : DemoRunner(m) {}

  uint8_t scale_col(int val, int lo, int hi) {
    if (val < lo) return 0;
    if (val > hi) return 255;
    return 255 * (val - lo) / (hi - lo);
  }

  void Run() override {
    const int cent_x = m->get_columns() / 2;
    const int cent_y = m->get_rows() / 2;

    // The square to rotate (inner square + black frame) needs to cover the
    // whole area, even if diagonal. Thus, when rotating, the outer pixels from
    // the previous frame are cleared.
    const int rotate_square = min(m->get_columns(), m->get_rows()) * 1.41;
    const int min_rotate = cent_x - rotate_square / 2;
    const int max_rotate = cent_x + rotate_square / 2;

    // The square to display is within the visible area.
    const int display_square = min(m->get_columns(), m->get_rows()) * 0.7;
    const int min_display = cent_x - display_square / 2;
    const int max_display = cent_x + display_square / 2;

    const float deg_to_rad = 2 * 3.14159265 / 360;
    int rotation = 0;
    while (1) {
      ++rotation;
      usleep(15 * 1000);
      rotation %= 360;
      for (int x = min_rotate; x < max_rotate; ++x) {
        for (int y = min_rotate; y < max_rotate; ++y) {
          float rot_x, rot_y;
          Rotate(x - cent_x, y - cent_x,
                 deg_to_rad * rotation, &rot_x, &rot_y);
          if (x >= min_display && x < max_display &&
              y >= min_display && y < max_display) { // within display square
            m->set_pixel(rot_x + cent_x, rot_y + cent_y, Matrix_RGB_t(
                               scale_col(x, min_display, max_display),
                               255 - scale_col(y, min_display, max_display),
                               scale_col(y, min_display, max_display)));
          } else {
            // black frame.
            m->set_pixel(rot_x + cent_x, rot_y + cent_y, Matrix_RGB_t(0, 0, 0));
          }
        }
      }
      m->send_frame();
    }
  }

private:
  void Rotate(int x, int y, float angle,
              float *new_x, float *new_y) {
    *new_x = x * cosf(angle) - y * sinf(angle);
    *new_y = x * sinf(angle) + y * cosf(angle);
  }
};

class ImageScroller : public DemoRunner {
public:
  // Scroll image with "scroll_jumps" pixels every "scroll_ms" milliseconds.
  // If "scroll_ms" is negative, don't do any scrolling.
  ImageScroller(Matrix *m, int scroll_jumps, int scroll_ms = 30) : DemoRunner(m), scroll_jumps_(scroll_jumps), scroll_ms_(scroll_ms), horizontal_position_(0) {  }

  // _very_ simplified. Can only read binary P6 PPM. Expects newlines in headers
  // Not really robust. Use at your own risk :)
  // This allows reload of an image while things are running, e.g. you can
  // live-update the content.
  bool LoadPPM(const char *filename) {
    FILE *f = fopen(filename, "r");
    // check if file exists
    if (f == NULL && access(filename, F_OK) == -1) {
      fprintf(stderr, "File \"%s\" doesn't exist\n", filename);
      return false;
    }
    if (f == NULL) return false;
    char header_buf[256];
    const char *line = ReadLine(f, header_buf, sizeof(header_buf));
#define EXIT_WITH_MSG(m) { fprintf(stderr, "%s: %s |%s", filename, m, line); \
      fclose(f); return false; }
    if (sscanf(line, "P6 ") == EOF)
      EXIT_WITH_MSG("Can only handle P6 as PPM type.");
    line = ReadLine(f, header_buf, sizeof(header_buf));
    int new_width, new_height;
    if (!line || sscanf(line, "%d %d ", &new_width, &new_height) != 2)
      EXIT_WITH_MSG("Width/height expected");
    int value;
    line = ReadLine(f, header_buf, sizeof(header_buf));
    if (!line || sscanf(line, "%d ", &value) != 1 || value != 255)
      EXIT_WITH_MSG("Only 255 for maxval allowed.");
    const size_t pixel_count = new_width * new_height;
    Pixel *new_image = new Pixel [ pixel_count ];
    assert(sizeof(Pixel) == 3);   // we make that assumption.
    if (fread(new_image, sizeof(Pixel), pixel_count, f) != pixel_count) {
      line = "";
      EXIT_WITH_MSG("Not enough pixels read.");
    }
#undef EXIT_WITH_MSG
    fclose(f);
    fprintf(stderr, "Read image '%s' with %dx%d\n", filename,
            new_width, new_height);
    horizontal_position_ = 0;
    new_image_.Delete();  // in case we reload faster than is picked up
    new_image_.image = new_image;
    new_image_.width = new_width;
    new_image_.height = new_height;
    return true;
  }

  void Run() override {
    const int screen_height = m->get_rows();
    const int screen_width = m->get_columns();
    while (true) {
      {
        if (new_image_.IsValid()) {
          current_image_.Delete();
          current_image_ = new_image_;
          new_image_.Reset();
        }
      }
      if (!current_image_.IsValid()) {
        usleep(100 * 1000);
        continue;
      }
      for (int x = 0; x < screen_width; ++x) {
        for (int y = 0; y < screen_height; ++y) {
          const Pixel &p = current_image_.getPixel(
            (horizontal_position_ + x) % current_image_.width, y);
          m->set_pixel(x, y, Matrix_RGB_t(p.red, p.green, p.blue));
        }
      }
      m->send_frame();
      horizontal_position_ += scroll_jumps_;
      if (horizontal_position_ < 0) horizontal_position_ = current_image_.width;
      if (scroll_ms_ <= 0) {
        // No scrolling. We don't need the image anymore.
        current_image_.Delete();
      } else {
        usleep(scroll_ms_ * 1000);
      }
    }
  }

private:
  struct Pixel {
    Pixel() : red(0), green(0), blue(0){}
    uint8_t red;
    uint8_t green;
    uint8_t blue;
  };

  struct Image {
    Image() : width(-1), height(-1), image(NULL) {}
    ~Image() { Delete(); }
    void Delete() { delete [] image; Reset(); }
    void Reset() { image = NULL; width = -1; height = -1; }
    inline bool IsValid() { return image && height > 0 && width > 0; }
    const Pixel &getPixel(int x, int y) {
      static Pixel black;
      if (x < 0 || x >= width || y < 0 || y >= height) return black;
      return image[x + width * y];
    }

    int width;
    int height;
    Pixel *image;
  };

  // Read line, skip comments.
  char *ReadLine(FILE *f, char *buffer, size_t len) {
    char *result;
    do {
      result = fgets(buffer, len, f);
    } while (result != NULL && result[0] == '#');
    return result;
  }

  const int scroll_jumps_;
  const int scroll_ms_;

  // Current image is only manipulated in our thread.
  Image current_image_;
  Image new_image_;

  int32_t horizontal_position_;
};

// Abelian sandpile
// Contributed by: Vliedel
class Sandpile : public DemoRunner {
public:
  Sandpile(Matrix *m, int delay_ms = 50) : DemoRunner(m), delay_ms_(delay_ms) {
    width_ = m->get_columns() - 1; // We need an odd width
    height_ = m->get_rows() - 1; // We need an odd height

    // Allocate memory
    values_ = new int*[width_];
    for (int x=0; x<width_; ++x) {
      values_[x] = new int[height_];
    }
    newValues_ = new int*[width_];
    for (int x=0; x<width_; ++x) {
      newValues_[x] = new int[height_];
    }

    // Init values
    srand(time(NULL));
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        values_[x][y] = 0;
      }
    }
  }

  ~Sandpile() {
    for (int x=0; x<width_; ++x) {
      delete [] values_[x];
    }
    delete [] values_;
    for (int x=0; x<width_; ++x) {
      delete [] newValues_[x];
    }
    delete [] newValues_;
  }

  void Run() override {
    while (1) {
      // Drop a sand grain in the centre
      values_[width_/2][height_/2]++;
      updateValues();

      for (int x=0; x<width_; ++x) {
        for (int y=0; y<height_; ++y) {
          switch (values_[x][y]) {
          case 0:
            m->set_pixel(x, y, Matrix_RGB_t(0, 0, 0));
            break;
          case 1:
            m->set_pixel(x, y, Matrix_RGB_t(0, 0, 200));
            break;
          case 2:
            m->set_pixel(x, y, Matrix_RGB_t(0, 200, 0));
            break;
          case 3:
            m->set_pixel(x, y, Matrix_RGB_t(150, 100, 0));
            break;
          default:
            m->set_pixel(x, y, Matrix_RGB_t(200, 0, 0));
          }
        }
      }
      m->send_frame();
      usleep(delay_ms_ * 1000); // ms
    }
  }

private:
  void updateValues() {
    // Copy values to newValues
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        newValues_[x][y] = values_[x][y];
      }
    }

    // Update newValues based on values
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        if (values_[x][y] > 3) {
          // Collapse
          if (x>0)
            newValues_[x-1][y]++;
          if (x<width_-1)
            newValues_[x+1][y]++;
          if (y>0)
            newValues_[x][y-1]++;
          if (y<height_-1)
            newValues_[x][y+1]++;
          newValues_[x][y] -= 4;
        }
      }
    }
    // Copy newValues to values
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        values_[x][y] = newValues_[x][y];
      }
    }
  }

  int width_;
  int height_;
  int** values_;
  int** newValues_;
  int delay_ms_;
};


// Conway's game of life
// Contributed by: Vliedel
class GameLife : public DemoRunner {
public:
  GameLife(Matrix *m, int delay_ms=500, bool torus=true) : DemoRunner(m), delay_ms_(delay_ms), torus_(torus) {
    width_ = m->get_columns();
    height_ = m->get_rows();

    // Allocate memory
    values_ = new int*[width_];
    for (int x=0; x<width_; ++x) {
      values_[x] = new int[height_];
    }
    newValues_ = new int*[width_];
    for (int x=0; x<width_; ++x) {
      newValues_[x] = new int[height_];
    }

    // Init values randomly
    srand(time(NULL));
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        values_[x][y]=rand()%2;
      }
    }
    r_ = rand()%255;
    g_ = rand()%255;
    b_ = rand()%255;

    if (r_<150 && g_<150 && b_<150) {
      int c = rand()%3;
      switch (c) {
      case 0:
        r_ = 200;
        break;
      case 1:
        g_ = 200;
        break;
      case 2:
        b_ = 200;
        break;
      }
    }
  }

  ~GameLife() {
    for (int x=0; x<width_; ++x) {
      delete [] values_[x];
    }
    delete [] values_;
    for (int x=0; x<width_; ++x) {
      delete [] newValues_[x];
    }
    delete [] newValues_;
  }

  void Run() override {
    while (1) {

      updateValues();

      for (int x=0; x<width_; ++x) {
        for (int y=0; y<height_; ++y) {
          if (values_[x][y])
            m->set_pixel(x, y, Matrix_RGB_t(r_, g_, b_));
          else
            m->set_pixel(x, y, Matrix_RGB_t(0, 0, 0));
        }
      }
      m->send_frame();
      usleep(delay_ms_ * 1000); // ms
    }
  }

private:
  int numAliveNeighbours(int x, int y) {
    int num=0;
    if (torus_) {
      // Edges are connected (torus)
      num += values_[(x-1+width_)%width_][(y-1+height_)%height_];
      num += values_[(x-1+width_)%width_][y                    ];
      num += values_[(x-1+width_)%width_][(y+1        )%height_];
      num += values_[(x+1       )%width_][(y-1+height_)%height_];
      num += values_[(x+1       )%width_][y                    ];
      num += values_[(x+1       )%width_][(y+1        )%height_];
      num += values_[x                  ][(y-1+height_)%height_];
      num += values_[x                  ][(y+1        )%height_];
    }
    else {
      // Edges are not connected (no torus)
      if (x>0) {
        if (y>0)
          num += values_[x-1][y-1];
        if (y<height_-1)
          num += values_[x-1][y+1];
        num += values_[x-1][y];
      }
      if (x<width_-1) {
        if (y>0)
          num += values_[x+1][y-1];
        if (y<31)
          num += values_[x+1][y+1];
        num += values_[x+1][y];
      }
      if (y>0)
        num += values_[x][y-1];
      if (y<height_-1)
        num += values_[x][y+1];
    }
    return num;
  }

  void updateValues() {
    // Copy values to newValues
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        newValues_[x][y] = values_[x][y];
      }
    }
    // update newValues based on values
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        int num = numAliveNeighbours(x,y);
        if (values_[x][y]) {
          // cell is alive
          if (num < 2 || num > 3)
            newValues_[x][y] = 0;
        }
        else {
          // cell is dead
          if (num == 3)
            newValues_[x][y] = 1;
        }
      }
    }
    // copy newValues to values
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        values_[x][y] = newValues_[x][y];
      }
    }
  }

  int** values_;
  int** newValues_;
  int delay_ms_;
  int r_;
  int g_;
  int b_;
  int width_;
  int height_;
  bool torus_;
};

// Langton's ant
// Contributed by: Vliedel
class Ant : public DemoRunner {
public:
  Ant(Matrix *m, int delay_ms=500) : DemoRunner(m), delay_ms_(delay_ms) {
    numColors_ = 4;
    width_ = m->get_columns();
    height_ = m->get_rows();
    values_ = new int*[width_];
    for (int x=0; x<width_; ++x) {
      values_[x] = new int[height_];
    }
  }

  ~Ant() {
    for (int x=0; x<width_; ++x) {
      delete [] values_[x];
    }
    delete [] values_;
  }

  void Run() override {
    antX_ = width_/2;
    antY_ = height_/2-3;
    antDir_ = 0;
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        values_[x][y] = 0;
        updatePixel(x, y);
      }
    }

    while (1) {
      // LLRR
      switch (values_[antX_][antY_]) {
      case 0:
      case 1:
        antDir_ = (antDir_+1+4) % 4;
        break;
      case 2:
      case 3:
        antDir_ = (antDir_-1+4) % 4;
        break;
      }

      values_[antX_][antY_] = (values_[antX_][antY_] + 1) % numColors_;
      int oldX = antX_;
      int oldY = antY_;
      switch (antDir_) {
      case 0:
        antX_++;
        break;
      case 1:
        antY_++;
        break;
      case 2:
        antX_--;
        break;
      case 3:
        antY_--;
        break;
      }
      updatePixel(oldX, oldY);
      if (antX_ < 0 || antX_ >= width_ || antY_ < 0 || antY_ >= height_)
        return;
      updatePixel(antX_, antY_);
      m->send_frame();
      usleep(delay_ms_ * 1000);
    }
  }

private:
  void updatePixel(int x, int y) {
    switch (values_[x][y]) {
    case 0:
      m->set_pixel(x, y, Matrix_RGB_t(200, 0, 0));
      break;
    case 1:
      m->set_pixel(x, y, Matrix_RGB_t(0, 200, 0));
      break;
    case 2:
      m->set_pixel(x, y, Matrix_RGB_t(0, 0, 200));
      break;
    case 3:
      m->set_pixel(x, y, Matrix_RGB_t(150, 100, 0));
      break;
    }
    if (x == antX_ && y == antY_)
      m->set_pixel(x, y, Matrix_RGB_t(0, 0, 0));
  }

  int numColors_;
  int** values_;
  int antX_;
  int antY_;
  int antDir_; // 0 right, 1 up, 2 left, 3 down
  int delay_ms_;
  int width_;
  int height_;
};

static int usage(const char *progname) {
  fprintf(stderr, "usage: %s <options> -D <demo num>\n", progname);
  fprintf(stderr, "Options:\n");
  fprintf(stderr,
          "\t-D <demo num>\t-\tAlways needs to be set\n");
  fprintf(stderr, 
          "\t0  - some rotating square\n"
          "\t1  - forward scrolling an image (-m <scroll-ms>)\n"
          "\t2  - backward scrolling an image (-m <scroll-ms>)\n"
          "\t4  - Pulsing color\n"
          "\t5  - Grayscale Block\n"
          "\t6  - Abelian sandpile model (-m <time-step-ms>)\n"
          "\t7  - Conway's game of life (-m <time-step-ms>)\n"
          "\t8  - Langton's ant (-m <time-step-ms>)\n"
          "\t11 - Brightness pulse generator\n");
  fprintf(stderr, "Example:\n\t%s -D 0\n", progname);
  return 1;
}

int main(int argc, char *argv[]) {
  int opt;
  int demo = -1;
  int scroll_ms = 30;
  DemoRunner *demo_runner;
  const char *demo_parameter = NULL;
  Matrix *m = new Matrix(0);
  
  while ((opt = getopt(argc, argv, "dD:r:P:c:p:b:m:LR:")) != -1) {
    switch (opt) {
    case 'D':
      demo = atoi(optarg);
      break;

    case 'm':
      scroll_ms = atoi(optarg);
      break;

    default: /* '?' */
      return usage(argv[0]);
    }
  }
  
  if (optind < argc) {
    demo_parameter = argv[optind];
  }

  if (demo < 0) {
    fprintf(stderr, TERM_ERR "Expected required option -D <demo>\n" TERM_NORM);
    return usage(argv[0]);
  }

  // The DemoRunner objects are filling
  // the matrix continuously.
  switch (demo) {
  case 0:
    demo_runner = new RotatingBlockGenerator(m);
    break;
  case 1:
  case 2:
    if (demo_parameter) {
      ImageScroller *scroller = new ImageScroller(m, demo == 1 ? 1 : -1, scroll_ms);
      if (!scroller->LoadPPM(demo_parameter))
        return 1;
      demo_runner = scroller;
    } else {
      fprintf(stderr, "Demo %d Requires PPM image as parameter\n", demo);
      return 1;
    }
    break;
  case 4:
    demo_runner = new ColorPulseGenerator(m);
    break;
  case 5:
    demo_runner = new GrayScaleBlock(m);
    break;
  case 6:
    demo_runner = new Sandpile(m, scroll_ms);
    break;
  case 7:
    demo_runner = new GameLife(m, scroll_ms);
    break;
  case 8:
    demo_runner = new Ant(m, scroll_ms);
    break;  
  case 11:
    demo_runner = new BrightnessPulseGenerator(m);
    break;
  default:
    return usage(argv[0]);
  }

  demo_runner->Run();
  return 0;
}
