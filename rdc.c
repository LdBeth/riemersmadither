#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <netpbm/ppm.h>      // requires netpbm/ppm.h and linking with -lnetpbm
#include "gilbert.h"  // requires gilbert.h and gilbert.c

/*
 * Macro to index into our flat errors array.
 * We allocate errors as a 1D array of size: cols * (rows+n) * 3,
 * where errors[x][y][c] is at index: x * ((rows+n)*3) + y*3 + c.
 */
#define ERR(errors, cols, rows, n, x, y, c) (errors[((x) * ((rows)+(n)) + (y)) * 3 + (c)])

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "ifile pfile ofile\n");
        return 1;
    }
    const char *ifile = argv[1];
    const char *pfile = argv[2];
    const char *ofile = argv[3];

    /* --- Read the palette image --- */
    int pcols = 0, prows = 0;
    pixval pmaxval = 0;
    FILE *fp = fopen(pfile, "r");
    if (!fp) {
        fprintf(stderr, "Error opening palette file '%s'\n", pfile);
        return 1;
    }
    /* ppm_readppm returns a pointer-to-pointer to pixel.
       We assume the palette image is one row high. */
    pixel **pal = ppm_readppm(fp, &pcols, &prows, &pmaxval);
    fclose(fp);
    if (prows <= 0 || pcols <= 0) {
        fprintf(stderr, "Invalid palette image\n");
        return 1;
    }
    // Copy the palette row into two arrays:
    // - qpixel: used for the final output lookup
    // - palette: used for distance computations
    pixel *qpixel = malloc(pcols * sizeof(pixel));
    pixel *palette = malloc(pcols * sizeof(pixel));
    for (int i = 0; i < pcols; i++) {
        qpixel[i]   = pal[0][i];
        palette[i]  = pal[0][i];
    }
    ppm_freearray((void**)pal, prows);

    /* --- Read the input image --- */
    int cols = 0, rows = 0;
    pixval maxval = 0;
    fp = fopen(ifile, "r");
    if (!fp) {
        fprintf(stderr, "Error opening input file '%s'\n", ifile);
        return 1;
    }
    pixel **image = ppm_readppm(fp, &cols, &rows, &maxval);
    fclose(fp);
    if (cols <= 0 || rows <= 0) {
        fprintf(stderr, "Invalid input image\n");
        return 1;
    }

    /* --- Generate the Gilbert ordering --- */
    int totalPixels = cols * rows;
    int *gl = malloc(totalPixels * sizeof(int));
    gilbert(gl, cols, rows);
    // In the Chapel code an index array idx[x][y] is built as: idx(x,y) = gl[x*rows+y]
    // Here we use gl directly with the same indexing.

    /* --- Set up parameters for error diffusion --- */
    const int n = 32;
    const double r = 0.125;
    int weight[n];
    for (int i = 0; i < n; i++) {
        weight[i] = (int)round(pow(r, -((double)i / (n - 1))));
    }

    /* Allocate errors array with dimensions: [cols][rows+n][3] */
    int errors_size = cols * (rows + n) * 3;
    int *errors = calloc(errors_size, sizeof(int));
    if (!errors) {
        fprintf(stderr, "Allocation error for errors array\n");
        return 1;
    }

    /* --- Process each pixel in the gilbert order --- */
    for (int x = 0; x < cols; x++) {
        for (int y = 0; y < rows; y++) {
            // Get the ordering index from gl
            int loc = gl[x * rows + y];
            int j = loc / rows;  // column index in the image
            int k = loc % rows;  // row index in the image

            pixel pix = image[k][j];
            int rgb[3] = { (int)pix.r, (int)pix.g, (int)pix.b };

            /* Calculate adjustment from errors:
               For each color channel, sum over i=0..n-1: errors[x][y+i][c] * weight[i],
               then divide by n. */
            int adj[3] = {0, 0, 0};
            for (int i = 0; i < n; i++) {
                for (int c = 0; c < 3; c++) {
                    adj[c] += ERR(errors, cols, rows, n, x, y + i, c) * weight[i];
                }
            }
            for (int c = 0; c < 3; c++) {
                adj[c] /= n;
            }

            int q[3];
            for (int c = 0; c < 3; c++) {
                q[c] = rgb[c] + adj[c];
            }

            /* Find the best palette match for q using a weighted distance:
               distance = sqrt( 3*(dr)^2 + 4*(dg)^2 + 2*(db)^2 )
               (Note: for minimization, the square root can be omitted.) */
            int best_index = 0;
            int min_distance = INT_MAX;
            for (int i = 0; i < pcols; i++) {
                int dr = (int)palette[i].r - q[0];
                int dg = (int)palette[i].g - q[1];
                int db = (int)palette[i].b - q[2];
                int dist_sq = 3 * dr * dr + 4 * dg * dg + 2 * db * db;
                if (dist_sq < min_distance) {
                    min_distance = dist_sq;
                    best_index = i;
                }
            }

            // Set the quantized pixel from the original palette (qpixel)
            image[k][j] = qpixel[best_index];

            // Compute the error for each channel and store it in errors[x][y+n]
            int err[3];
            err[0] = rgb[0] - (int)palette[best_index].r;
            err[1] = rgb[1] - (int)palette[best_index].g;
            err[2] = rgb[2] - (int)palette[best_index].b;
            for (int c = 0; c < 3; c++) {
                ERR(errors, cols, rows, n, x, y + n, c) = err[c];
            }
        }
    }

    /* --- Write out the resulting image --- */
    fp = fopen(ofile, "w");
    if (!fp) {
        fprintf(stderr, "Error opening output file '%s'\n", ofile);
        return 1;
    }
    ppm_writeppm(fp, image, cols, rows, maxval, 0);
    fclose(fp);

    /* --- Cleanup --- */
    free(gl);
    free(errors);
    free(palette);
    free(qpixel);
    ppm_freearray((void**)image, rows);

    return 0;
}
