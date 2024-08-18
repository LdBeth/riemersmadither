// cc -shared -Wl,-install_name,gilbert.dylib -o gilbert.dylib gilbert.c
#include <stdlib.h>
#include <string.h>

static int *buffer;
// static int *input;
static int bidx;
static int xbase;

static int sign(int x) {
  return (x > 0) - (x < 0);
}

static void writeidx(int x, int y){
  buffer[bidx] = x * xbase + y;
  bidx++;
  return;
}

static void generate2d(int x, int y, int ax, int ay, int bx, int by) {
  int w = abs(ax + ay);
  int h = abs(bx + by);
  int dax = sign(ax);
  int day = sign(ay);
  int dbx = sign(bx);
  int dby = sign(by);

  if (h == 1) {
    for (int i = 0; i < w; i++){
      writeidx(x, y);
      x += dax;
      y += day;
    }
    return;
  }

  if (w == 1) {
    for (int i = 0; i < h; i++){
      writeidx(x, y);
      x += dbx;
      y += dby;
    }
    return;
  }

  int ax2 = ax / 2;
  int ay2 = ay / 2;
  int bx2 = bx / 2;
  int by2 = by / 2;

  int w2 = abs(ax2 + ay2);
  int h2 = abs(bx2 + by2);

  if ((2 * w) > (3 * h)) {
    if ((w2 % 2) && (w > 2)) {
      ax2 += dax;
      ay2 += day;
    }
    generate2d(x, y, ax2, ay2, bx, by);
    generate2d(x+ax2, y+ay2, ax-ax2, ay-ay2, bx, by);
    return;
  } else {
    if ((h2 % 2) && (h > 2)) {
      bx2 += dbx;
      by2 += dby;
    }
    generate2d(x, y, bx2, by2, ax2, ay2);
    generate2d(x+bx2, y+by2, ax, ay, bx-bx2, by-by2);
    generate2d(x+(ax-dax)+(bx2-dbx), y+(ay-day)+(by2-dby),
               -bx2, -by2, -(ax-ax2), -(ay-ay2));
    return;
  }
}

void gilbert(int *out, int width, int height) {
  bidx = 0;
  buffer = out;
  xbase = height;
  if (width >= height) {
    generate2d(0, 0, width, 0, 0, height);
  } else {
    generate2d(0, 0, 0, height, width, 0);
  }
  return;
}
