#include "raytracer.h"
#include <omp.h>

/*
 * Calcula el color de un píxel con antialiasing (box filter).
 * Equivalente a antialiasingPixelColor() del Java.
 */
static ColorRGB pixel_color(const Scene *scene, int px, int py) {
    int     samples = scene->samplesPerPixel;
    double *pts;
    int     count;
    sampling_grid(1.0, samples, &pts, &count);

    double r = 0.0, g = 0.0, b = 0.0;
    for (int k = 0; k < count; k++) {
        double offX = pts[2*k];
        double offY = pts[2*k+1];
        Ray ray = camera_build_ray(&scene->camera, px, py, offX, offY);
        ColorRGB c = shade(ray, scene, scene->rayMaxBounces);
        r += c.r;
        g += c.g;
        b += c.b;
    }
    free(pts);

    return color((float)(r / count),
                 (float)(g / count),
                 (float)(b / count));
}

/* ───────────────────────────────────────────────
   VERSIÓN SERIAL
─────────────────────────────────────────────── */
Image *render_serial(const Scene *scene) {
    int W = scene->camera.imageWidth;
    int H = scene->camera.imageHeight;
    Image *img = image_create(W, H);
    if (!img) return NULL;

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            image_set_pixel(img, x, y, pixel_color(scene, x, y));
        }
    }
    return img;
}

/* ───────────────────────────────────────────────
   VERSIÓN PARALELA  (OpenMP)

   Estrategia: paralelismo sobre filas (igual que
   en el Java con división de renglones por hilo).
   OpenMP reparte las filas dinámicamente entre los
   hilos disponibles; cada píxel dentro de una fila
   es independiente, así que también paralelizamos
   el bucle interior con collapse(2).

   No hay condición de carrera: cada hilo escribe en
   posiciones distintas del buffer de la imagen.
─────────────────────────────────────────────── */
Image *render_parallel(const Scene *scene, int numThreads) {
    int W = scene->camera.imageWidth;
    int H = scene->camera.imageHeight;
    Image *img = image_create(W, H);
    if (!img) return NULL;

    omp_set_num_threads(numThreads);

    /*
     * collapse(2) fusiona los dos bucles en uno solo,
     * aumentando la granularidad del trabajo repartido.
     * schedule(dynamic, 4) reparte bloques de 4 filas
     * a la vez para equilibrar carga (útil cuando hay
     * objetos que provocan tiempos de render dispares).
     */
    #pragma omp parallel for collapse(2) schedule(dynamic, 4)
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            ColorRGB c = pixel_color(scene, x, y);
            image_set_pixel(img, x, y, c);
        }
    }

    return img;
}
