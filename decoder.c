#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <png.h>
#include <stdint.h>
#include <string.h>

// This function actually writes out the PNG image file. The string 'title' is
// also written into the image file
int write_png(char *filename, uint32_t width, uint32_t height, const uint8_t *buffer, char *title);

uint8_t *decode_raw12(const uint8_t *input_buffer, size_t input_len);

int main(int argc, char *argv[]) {
    // Make sure that the output filename argument has been provided
    if (argc != 3) {
        fprintf(stderr, "Please specify output file\n");
        return 1;
    }

    // Specify an output image size
    uint32_t width = 1280;
    uint32_t height = 800;

    size_t input_len = (size_t) (width * height * 1.5) * sizeof(uint8_t);
    uint8_t *input_buffer = malloc(input_len);
    FILE *input = fopen("/home/azhidkov/0my/Work/IP/Labracon/lpx/tmp/test-ar0144-181019-me2-21", "rb");
    fread(input_buffer, input_len, 1, input);
    fclose(input);

    // Create a test image - in this case a Mandelbrot Set fractal
    // The output is a 1D array of floats, length: width * height
    printf("Creating Image\n");
    uint8_t *buffer = decode_raw12(input_buffer, input_len);
    if (buffer == NULL) {
        return 1;
    }

    // Save the image to a PNG file
    // The 'title' string is stored as part of the PNG file
    printf("Saving PNG\n");
    int result = write_png(argv[1], width, height, buffer, argv[1]);

    // Free up the memorty used to store the image
    free(buffer);

    return result;
}

int write_png(char *filename, uint32_t width, uint32_t height, const uint8_t *buffer, char *title) {
    int code = 0;
    FILE *fp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytep row = NULL;

    // Open file for writing (binary mode)
    fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Could not open file %s for writing\n", filename);
        code = 1;
        goto finalise;
    }

    // Initialize write structure
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fprintf(stderr, "Could not allocate write struct\n");
        code = 1;
        goto finalise;
    }

    // Initialize info structure
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fprintf(stderr, "Could not allocate info struct\n");
        code = 1;
        goto finalise;
    }

    // Setup Exception handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error during png creation\n");
        code = 1;
        goto finalise;
    }

    png_init_io(png_ptr, fp);

    // Write header (8 bit colour depth)
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    // Set title
    if (title != NULL) {
        png_text title_text;
        title_text.compression = PNG_TEXT_COMPRESSION_NONE;
        title_text.key = "Title";
        title_text.text = title;
        png_set_text(png_ptr, info_ptr, &title_text, 1);
    }

    png_write_info(png_ptr, info_ptr);

    // Allocate memory for one row (3 bytes per pixel - RGB)
    row = (png_bytep) malloc(3 * width * sizeof(png_byte));

    // Write image data
    int x, y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            row[x * 3] = buffer[y * width + x];
            row[x * 3 + 1] = buffer[y * width + x];
            row[x * 3 + 2] = buffer[y * width + x];
        }
        png_write_row(png_ptr, row);
    }

    // End write
    png_write_end(png_ptr, NULL);

    finalise:
    if (fp != NULL) fclose(fp);
    if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
    if (row != NULL) free(row);

    return code;
}

uint8_t *decode_raw12(const uint8_t *input_buffer, size_t input_len) {
    uint8_t *buffer = (uint8_t *) malloc((size_t) (input_len / 1.5) * sizeof(uint8_t));
    if (buffer == NULL) {
        fprintf(stderr, "Could not create image buffer\n");
        return NULL;
    }

    for (int i = 0, j = 0; i < input_len; i += 3, j += 2) {
        buffer[j] = (uint8_t) ((input_buffer[i] << 4 & 0xFF) | (input_buffer[i + 2] & 0xF));
        buffer[j + 1] = (uint8_t) ((input_buffer[i + 1] << 4 & 0xFF) | (input_buffer[i + 2] >> 4));
    }

    return buffer;
}
