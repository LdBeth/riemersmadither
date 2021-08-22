use SysCTypes;
use CPtr;

config const ifile, pfile, ofile : string;

if ifile.size == 0 || pfile.size == 0 || ofile.size == 0 then {
  writeln("no file");
  exit(1);
 }

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

require "gilbert.h", "gilbert.c";

extern proc gilbert(a : c_int, b : c_int) : c_ptr(c_int);

var pcols: c_int = 0;
var prows: c_int = 0;
var pmaxval : pixval = 0;

var file : c_ptr(FILE) = fopen(pfile.c_str(), "r");
var pal = ppm_readppm(file,
                      pcols,
                      prows,
                      pmaxval);
fclose(file);

const DPX : domain(1) = {0..#pcols};
const DP : domain(2) = {0..#pcols, 1..3};


var p : [DP] pixval;
var px : [DPX] pixel;
for i in 0..#pcols {
  const pix = pal[0][i];
  px[i]=pix;
  p[i, 1..3] = [pix.r, pix.g, pix.b];
}
ppm_freearray(pal, prows);

const (palette, qpixel) = (p, px);

//writeln("read P.");
// writeln("I cols: ",cols," rows: ",rows," max: ",maxval);

//writeln("P cols: ",pcols," rows: ",prows," max: ",pmaxval);
//writeln(palette);

var cols: c_int = 0;
var rows: c_int = 0;
var maxval : pixval = 0;

file = fopen(ifile.c_str(), "r");
var image = ppm_readppm(file,
                        cols,
                        rows,
                        maxval);
fclose(file);

//writeln("read I.");

var idx : [0..#cols, 0..#rows] c_int;

var gl = gilbert(cols,rows);

forall (x,y) in {0..#cols, 0..#rows} do
  idx(x,y) = gl[x*rows+y];

// writeln(idx);
c_free(gl);

//writeln("gen curve.");

param n : int(32) = 32;
param r : real = 0.125;

proc init_weight(n : int(32)) {
  var w : [0..#n] int(32);
  for i in 0..#n do
    w[i] = round(r ** (- (i: real(32)) / (n - 1): real(32))) : int(32);
  return w;
 }

const weight : [0..#n] int(32) = init_weight(n);

const RGB : domain(1) = {1..3};
const ED : domain(3) = {0..#cols, 0..#(rows+n), 1..3};
var errors : [ED] int(32);

inline proc calc_adj(x : int, y :int) : [RGB] int(32) {
  var a : [RGB] int(32);
  forall i in 0..#n do
    a += (errors[x, y+i, 1..3] * weight[i]) : int(32);
  return a/n;
}

forall x in 0..#cols {
  for y in 0..#rows {
    const loc = idx(x,y);
    const j = loc / rows;
    const k = loc % rows;
    const pix = image[k][j];
    const rgb : [RGB] c_uint = [pix.r,pix.g,pix.b];
    const adj = calc_adj(x, y);
    const q : [RGB] c_uint = for i in RGB do (rgb[i] + adj[i]) : c_uint;
    // quant
    const a : [DPX] int(32) = forall i in DPX do
      // [2,4,3] if prefers blue
    ((+ reduce ([3,4,2] * (palette[i, 1..3] - q) ** 2))** 0.5): int(32);
    const (min,i) = minloc reduce zip(a, DPX);
    image[k][j] = qpixel[i];
    const err = for a in RGB do (rgb[a]-palette[i, a]) : int(32);
    errors[x,y+n, 1..3] = err;
  }
}

file = fopen(ofile.c_str(), "w");
ppm_writeppm(file, image, cols, rows, maxval, 0);
fclose(file);

ppm_freearray(image, rows);