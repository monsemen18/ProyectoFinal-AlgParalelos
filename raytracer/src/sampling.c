#include "raytracer.h"

/*
 * Genera una cuadrícula de n×n puntos dentro de un cuadrado de lado `size`.
 * Equivalente a SquareSamplingPoints.obtainSamplingPoints() del Java.
 *
 * *pts  → array de pares [x, y] almacenados de forma plana: pts[2*i], pts[2*i+1]
 * *count → número de puntos generados (n*n)
 *
 * El llamador es responsable de liberar *pts con free().
 */
void sampling_grid(double size, int samples, double **pts, int *count) {
    int n    = (int)ceil(sqrt((double)samples));
    double step = size / n;
    int total   = n * n;

    double *arr = (double *)malloc(total * 2 * sizeof(double));
    int idx = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            arr[idx++] = (i + 0.5) * step;
            arr[idx++] = (j + 0.5) * step;
        }
    }
    *pts   = arr;
    *count = total;
}
