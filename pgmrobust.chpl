use SysCTypes;
use CPtr;

config const ifile, ofile : string;

if ifile.size == 0 || ofile.size == 0 then {
  writeln("no file");
  exit(1);
 }

config const rad = 10;

require "netpbm/ppm.h", "-lnetpbm";

extern type FILE;
extern proc fopen(path : c_string, mode : c_string) : c_ptr(FILE);
extern proc fclose(stream : c_ptr(FILE)) : c_int;

extern type pixval = c_uint;

extern record pixel {
  var r, g, b : pixval;
}

extern proc ppm_readppm(fileP : c_ptr(FILE),
                        ref colsP : c_int,
                        ref rowsP : c_int,
                        ref maxvalP : pixval) : c_ptr(c_ptr(pixel));

extern proc ppm_writeppm(fileP : c_ptr(FILE),
                         pixels : c_ptr(c_ptr(pixel)),
                         cols : c_int,
                         rows : c_int,
                         maxval : pixval,
                         forceplain : c_int);

extern "pm_freearray" proc ppm_freearray(pixels : c_ptr(c_ptr(pixel)),
                                         rows : c_int);

var cols: c_int = 0;
var rows: c_int = 0;
var maxval : pixval = 0;
var file : c_ptr(FILE) = fopen(ifile.c_str(), "r");
var image = ppm_readppm(file,
                        cols,
                        rows,
                        maxval);
fclose(file);

const D: domain(2) = {0..#cols, 0..#rows};
const K: domain(2) = { -rad..rad, -rad..+rad };
var layer : [D] uint(32);

proc smooth(in arr : [D] uint(32))
{
  forall (x, y) in D {
    var maxv : uint(32) = 0;
    var minv : uint(32) = 255;
    for (i, j) in K {
       if !(i == 0 && j == 0) then
         if arr[x+i,y+j] > maxv then
           maxv = arr[x+i,y+j];
         if arr[x+i,y+j] < minv then
           minv = arr[x+i,y+j];
    }
    if arr[x,y] > maxv then
       arr[x,y] = maxv;
    else if arr[x,y] < minv then
       arr[x,y] = minv;
  }
}

forall (i,j) in layer.domain do
  layer(i,j) = image[j][i].r;

smooth(layer);

forall (i,j) in layer.domain do
  image[j][i].r = layer(i,j);

forall (i,j) in layer.domain do
  layer(i,j) = image[j][i].g;

smooth(layer);

forall (i,j) in layer.domain do
  image[j][i].g = layer(i,j);

forall (i,j) in layer.domain do
  layer(i,j) = image[j][i].b;

smooth(layer);

forall (i,j) in layer.domain do
  image[j][i].b = layer(i,j);

file = fopen(ofile.c_str(), "w");
ppm_writeppm(file, image, cols, rows, maxval, 0);
fclose(file);

ppm_freearray(image, rows);