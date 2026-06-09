#include "raytracer.h"
#include <omp.h>
#include <time.h>

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Uso: %s [opciones]\n"
        "  -s <scene.json>   Archivo de escena (default: stdin)\n"
        "  -o <output.ppm>   Imagen de salida   (default: output.ppm)\n"
        "  -t <n>            Número de hilos     (default: todos los CPUs)\n"
        "  --serial          Forzar modo serial (para benchmark)\n"
        "  --bench           Ejecutar ambos modos y reportar speedup\n"
        "  -h                Mostrar esta ayuda\n",
        prog);
}

/* Retorna segundos con alta resolución */
static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(int argc, char *argv[]) {
    const char *scene_file  = NULL;
    const char *output_file = "output.ppm";
    int  num_threads = omp_get_max_threads();
    int  serial_only = 0;
    int  bench_mode  = 0;

    /* ── Parseo de argumentos ── */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 && i+1 < argc) {
            scene_file = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i+1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0 && i+1 < argc) {
            num_threads = atoi(argv[++i]);
            if (num_threads < 1) num_threads = 1;
        } else if (strcmp(argv[i], "--serial") == 0) {
            serial_only = 1;
        } else if (strcmp(argv[i], "--bench") == 0) {
            bench_mode = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Argumento desconocido: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* ── Cargar escena ── */
    Scene scene;
    printf("Cargando escena...\n");
    if (scene_load_json(scene_file, &scene) != 0) {
        fprintf(stderr, "Error al cargar la escena.\n");
        return 2;
    }
    printf("  Resolución : %d × %d\n",
           scene.camera.imageWidth, scene.camera.imageHeight);
    printf("  Muestras/px: %d   Rebotes máx: %d\n",
           scene.samplesPerPixel, scene.rayMaxBounces);
    printf("  Primitivas : %d   Luces: %d   Materiales: %d\n",
           scene.numPrimitives, scene.numLights, scene.numMaterials);

    /* ── Modo benchmark: serial + paralelo ── */
    if (bench_mode) {
        printf("\n[BENCHMARK]\n");

        /* Serial */
        printf("  Renderizando (serial)...\n");
        double t0 = now_sec();
        Image *img_serial = render_serial(&scene);
        double t_serial   = now_sec() - t0;
        printf("  Tiempo serial  : %.3f s\n", t_serial);

        /* Paralelo */
        printf("  Renderizando (paralelo, %d hilos)...\n", num_threads);
        double t1 = now_sec();
        Image *img_par = render_parallel(&scene, num_threads);
        double t_par   = now_sec() - t1;
        printf("  Tiempo paralelo: %.3f s\n", t_par);

        printf("\n  Speedup        : %.2f×\n", t_serial / t_par);
        printf("  Eficiencia     : %.1f%%\n",
               100.0 * (t_serial / t_par) / num_threads);

        /* Guardar la imagen paralela */
        char serial_file[256];
        snprintf(serial_file, sizeof(serial_file), "serial_%s", output_file);
        image_write_ppm(img_serial, serial_file);
        image_write_ppm(img_par,    output_file);
        printf("\n  Imágenes guardadas:\n");
        printf("    Serial  → %s\n", serial_file);
        printf("    Paralelo→ %s\n", output_file);

        image_destroy(img_serial);
        image_destroy(img_par);
        return 0;
    }

    /* ── Modo normal ── */
    double t0 = now_sec();
    Image *img;

    if (serial_only) {
        printf("Renderizando (serial)...\n");
        img = render_serial(&scene);
    } else {
        printf("Renderizando (paralelo, %d hilos)...\n", num_threads);
        img = render_parallel(&scene, num_threads);
    }

    double elapsed = now_sec() - t0;
    if (!img) { fprintf(stderr, "Error al renderizar.\n"); return 3; }

    printf("Tiempo: %.3f s\n", elapsed);

    if (image_write_ppm(img, output_file) != 0) {
        fprintf(stderr, "Error al guardar imagen.\n");
        image_destroy(img);
        return 4;
    }
    printf("Imagen guardada en: %s\n", output_file);

    image_destroy(img);
    return 0;
}
