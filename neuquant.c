/* NeuQuant Neural-Net Quantization Algorithm adapted to NetPBM PPM input
 * Simplified C version that generates a 256-color palette from a PPM image
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <netpbm/ppm.h>

// #define NET_SIZE n      // Number of colors in the final palette
#define MAX_SIZE 256
#define INIT_RADIUS (NET_SIZE >> 3) // Initial radius
#define RADIUS_DEC 30
#define INIT_ALPHA 1024
#define ALPHA_DEC 30
#define PRIME1 499
#define PRIME2 491
#define PRIME3 487
#define PRIME4 503

typedef struct {
    int r, g, b;
    int freq;
    int bias;
} neuron_t;

int NET_SIZE;

static neuron_t network[MAX_SIZE];

// Initialize the network with evenly spaced colors
void init_net() {
    for (int i = 0; i < NET_SIZE; i++) {
        network[i].r = (i << 12) / NET_SIZE;
        network[i].g = (i << 12) / NET_SIZE;
        network[i].b = (i << 12) / NET_SIZE;
        network[i].freq = 1 << 16 / NET_SIZE;
        network[i].bias = 0;
    }
}

// Find best matching neuron index
int contest(int r, int g, int b) {
    int bestd = ~0U >> 1;
    int best = -1;
    for (int i = 0; i < NET_SIZE; i++) {
        int dr = network[i].r - r;
        int dg = network[i].g - g;
        int db = network[i].b - b;
        int d = abs(dr) + abs(dg) + abs(db);
        if (d < bestd) {
            bestd = d;
            best = i;
        }
    }
    return best;
}

// Alter neuron and its neighbors
void alter_neigh(int rad, int i, int r, int g, int b, int alpha) {
    for (int j = -rad; j <= rad; j++) {
        int k = i + j;
        if (k < 0 || k >= NET_SIZE) continue;
        network[k].r -= (alpha * (network[k].r - r)) / INIT_ALPHA;
        network[k].g -= (alpha * (network[k].g - g)) / INIT_ALPHA;
        network[k].b -= (alpha * (network[k].b - b)) / INIT_ALPHA;
    }
}

// Main training loop
void learn_ppm(pixel **image, int cols, int rows, int samplefac, int epochs) {
    int lengthcount = cols * rows;
    int step;

    if (lengthcount % PRIME1 != 0) step = PRIME1;
    else if (lengthcount % PRIME2 != 0) step = PRIME2;
    else if (lengthcount % PRIME3 != 0) step = PRIME3;
    else step = PRIME4;

    for (int epoch = 0; epoch < epochs; ++epoch) {
        int samplepixels = lengthcount / samplefac;
        int alpha = INIT_ALPHA;
        int radius = INIT_RADIUS;
        int alphadec = 30 + ((samplefac - 1) / 3);
        int rad = radius >> 6;
        int delta = samplepixels / 100;

        for (int i = 0, pix = 0; i < samplepixels; i++) {
            int y = pix / cols;
            int x = pix % cols;
            pixel p = image[y][x];

            int b = PPM_GETB(p) << 4;
            int g = PPM_GETG(p) << 4;
            int r = PPM_GETR(p) << 4;

            int j = contest(r, g, b);
            network[j].r -= (alpha * (network[j].r - r)) / INIT_ALPHA;
            network[j].g -= (alpha * (network[j].g - g)) / INIT_ALPHA;
            network[j].b -= (alpha * (network[j].b - b)) / INIT_ALPHA;

            alter_neigh(rad, j, r, g, b, alpha);

            pix += step;
            if (pix >= lengthcount) pix -= lengthcount;

            if (i % delta == 0) {
                alpha -= alpha / alphadec;
                rad = radius * ((samplepixels - i) / samplepixels);
                if (rad <= 1) rad = 0;
            }
        }
    }
}

// Build the color map
void build_colormap(unsigned char *colormap) {
    for (int i = 0; i < NET_SIZE; i++) {
        colormap[3 * i + 0] = network[i].b >> 4;
        colormap[3 * i + 1] = network[i].g >> 4;
        colormap[3 * i + 2] = network[i].r >> 4;
    }
}

// Public API for NetPBM input
void neuquant_generate_palette_ppm(pixel **image, int cols, int rows, unsigned char *palette_out) {
    init_net();
    learn_ppm(image, cols, rows, 5, 3);
    build_colormap(palette_out);
}


void write_palette_ppm(const char *filename, unsigned char *palette) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("fopen");
        return;
    }

    pixel **out = ppm_allocarray(NET_SIZE, 1);
    for (int i = 0; i < NET_SIZE; i++) {
        PPM_ASSIGN(out[0][i], palette[3*i+2], palette[3*i+1], palette[3*i+0]);
    }

    ppm_writeppm(fp, out, NET_SIZE, 1, PPM_MAXMAXVAL, 0);
    fclose(fp);
    ppm_freearray(out, 1);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: ncolors ifile ofile\n");
        return 1;
    }
    NET_SIZE = atoi(argv[1]);
    const char *ifile = argv[2];
    const char *ofile = argv[3];

    /* --- Read the input image --- */
    int cols, rows;
    pixval maxval;
    FILE *fp = fopen(ifile, "r");
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

    unsigned char out[MAX_SIZE * 3];
    neuquant_generate_palette_ppm(image, cols, rows, out);
    ppm_freearray(image, rows);
    write_palette_ppm(ofile, out);
}
