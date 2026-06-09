#include "raytracer.h"

/* ─── Crear imagen ─────────────────────────── */
Image *image_create(int w, int h) {
    Image *img = (Image *)malloc(sizeof(Image));
    if (!img) return NULL;
    img->width  = w;
    img->height = h;
    img->pixels = (ColorRGB *)calloc(w * h, sizeof(ColorRGB));
    if (!img->pixels) { free(img); return NULL; }
    return img;
}

/* ─── Liberar imagen ───────────────────────── */
void image_destroy(Image *img) {
    if (!img) return;
    free(img->pixels);
    free(img);
}

/* ─── Setters / getters ────────────────────── */
void image_set_pixel(Image *img, int x, int y, ColorRGB c) {
    if (x < 0 || x >= img->width || y < 0 || y >= img->height) return;
    img->pixels[y * img->width + x] = c;
}

ColorRGB image_get_pixel(Image *img, int x, int y) {
    return img->pixels[y * img->width + x];
}

/* ─── Escritura PPM (P6 binario) ───────────── */
int image_write_ppm(Image *img, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror("fopen"); return -1; }

    fprintf(f, "P6\n%d %d\n255\n", img->width, img->height);

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            ColorRGB c = image_get_pixel(img, x, y);
            unsigned char pixel[3];
            pixel[0] = (unsigned char)(clampf(c.r) * 255.0f);
            pixel[1] = (unsigned char)(clampf(c.g) * 255.0f);
            pixel[2] = (unsigned char)(clampf(c.b) * 255.0f);
            fwrite(pixel, 1, 3, f);
        }
    }

    fclose(f);
    return 0;
}
