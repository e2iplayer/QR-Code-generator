#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include "qrcodegen.h"

void save_png(const char *filename, const uint8_t *qrcode, int scale, int border, int *pWidth, int *pHeight) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Can not open file \"%s\" - write mode!\n", filename);
        exit(1);
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "png_create_write_struct failed!\n");
        fclose(file);
        exit(1);
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fprintf(stderr, "png_create_info_struct failed!\n");
        png_destroy_write_struct(&png, NULL);
        fclose(file);
        exit(1);
    }

    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "libpng write error!\n");
        png_destroy_write_struct(&png, &info);
        fclose(file);
        exit(1);
    }

    png_init_io(png, file);

    int size = qrcodegen_getSize(qrcode);
    int qr_width = size * scale;
    int qr_height = size * scale;
    int width = qr_width + 2 * border;
    int height = qr_height + 2 * border;
    *pWidth = width;
    *pHeight = height;

    png_set_IHDR(
        png,
        info,
        width, height,
        8, // color depth
        PNG_COLOR_TYPE_PALETTE, //PNG_COLOR_TYPE_GRAY,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    //png_set_compression_level(png, 9);
    png_color palette[2];
    palette[0].red = 0;
    palette[0].green = 0;
    palette[0].blue = 0;

    palette[1].red = 255;
    palette[1].green = 255;
    palette[1].blue = 255;

    png_set_PLTE(png, info, palette, 2);

    png_write_info(png, info);

    png_bytep *rows = malloc(height * sizeof(png_bytep));
    for (int y = 0; y < height; y++) {
        rows[y] = malloc(width * sizeof(png_byte));
        for (int x = 0; x < width; x++) {
            if (x < border || x >= width - border || y < border || y >= height - border) {
                rows[y][x] = 1;
            } else {
                int qr_x = (x - border) / scale;
                int qr_y = (y - border) / scale;
                rows[y][x] = qrcodegen_getModule(qrcode, qr_x, qr_y) ? 0 : 1; 
            }
        }
    }

    png_write_image(png, rows);
    png_write_end(png, NULL);

    for (int y = 0; y < height; y++) {
        free(rows[y]);
    }
    free(rows);

    png_destroy_write_struct(&png, &info);
    fclose(file);
}

int main(int argc, char** argv) {
    if (argc < 5) {
        fprintf(stderr, "e2iqr version: 1\n");
        fprintf(stderr, "Usage: \n");
        fprintf(stderr, "%s \"TEXT\" out_path.png scale border [errCorLvl 0-3]\n", argv[0]);
        return 1;
    }

    const char *text = argv[1]; // Tekst do zakodowania w QR
    char *out_path = argv[2];
    int scale;
    if (1 != sscanf(argv[3], "%d", &scale) || scale <= 0) {
        fprintf(stderr, "Wrong scale value!\n");
    }
    int border;
    if (1 != sscanf(argv[4], "%d", &border) || scale < 0) {
        fprintf(stderr, "Wrong border value!\n");
    }

    enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_HIGH; // Poziom korekcji błędów
    if (argc > 5) {
        int tmp;
        if (1 == sscanf(argv[5], "%d", &tmp)) {
            errCorLvl = tmp;
        }
    }

    // Bufor na kod QR
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

    // Generowanie kodu QR
    bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, errCorLvl, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
    if (!ok) {
        fprintf(stderr, "Fail to generate QR code\n");
        return 1;
    }

    int width = 0;
    int height = 0;
    save_png(out_path, qrcode, scale, border, width, height);

    printf("{\"path\":\"%s\", \"width\":\"\%d\", \"height\":\"\%d\"}\n", out_path, width, height);
    return 0;
}
