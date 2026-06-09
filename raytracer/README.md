# Raytracer en C con OpenMP

Implementación del algoritmo de raytracing en C, paralelizado con OpenMP.
Basado en la versión Java original con modelo de iluminación de Phong.

## Estructura del proyecto

```
raytracer/
├── include/
│   └── raytracer.h       # Tipos y API completa
├── src/
│   ├── main.c            # Punto de entrada, argumentos, benchmark
│   ├── raytracer.c       # render_serial() y render_parallel() con OpenMP
│   ├── shader.c          # Modelo Phong: difusa, especular, sombras, reflexión
│   ├── intersect.c       # Intersecciones rayo-esfera, rayo-plano, rayo-AABB
│   ├── camera.c          # Generación de rayos desde la cámara
│   ├── sampling.c        # Muestreo en cuadrícula (antialiasing y sombras suaves)
│   ├── scene_loader.c    # Parser JSON de escena (usa cJSON)
│   └── image.c           # Buffer de imagen y escritura PPM
├── Makefile
├── scene.json            # Escena de ejemplo
└── README.md
```

## Compilar

```bash
# Instalar dependencia (Ubuntu/Debian)
sudo apt-get install libcjson-dev

make
```

## Uso

```bash
# Render paralelo (todos los CPUs)
./raytracer -s scene.json -o output.ppm

# Render serial
./raytracer -s scene.json -o output.ppm --serial

# Render paralelo con N hilos
./raytracer -s scene.json -o output.ppm -t 4

# Benchmark: corre serial y paralelo, reporta speedup
./raytracer -s scene.json -o output.ppm --bench

# Leer escena desde stdin
cat scene.json | ./raytracer -o output.ppm
```

## Paralelización

La estrategia de paralelismo es sobre píxeles completos con `collapse(2)`:

```c
#pragma omp parallel for collapse(2) schedule(dynamic, 4)
for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
        ColorRGB c = pixel_color(scene, x, y);
        image_set_pixel(img, x, y, c);
    }
}
```

- **Sin condición de carrera**: cada hilo escribe en posiciones distintas del buffer.
- **`collapse(2)`**: fusiona los dos bucles en uno, aumentando la granularidad.
- **`schedule(dynamic, 4)`**: reparte bloques de 4 filas dinámicamente, equilibrando
  la carga cuando algunas zonas tienen más rebotes de reflexión que otras.

## Formato de escena (JSON)

Ver `scene.json` como referencia. Soporta:
- **Primitivas**: `sphere`, `plane`, `box` (AABB)
- **Luces**: `pointLight`, `directionalLight`, `surfaceLight`
- **Materiales**: color, difusa (kd), especular (ks), dureza, reflectividad
- **Cámara**: posición, dirección, FOV, distancia focal, resolución
- **Antialiasing**: `samplesPerPixel` muestras por píxel (box filter)
- **Sombras suaves**: `surfaceLight` con `samples` muestras en área
- **Reflexiones**: hasta `rayMaxBounces` rebotes

## Salida

Imagen en formato PPM (P6 binario). Convertir a PNG:
```bash
convert output.ppm output.png     # ImageMagick
ffmpeg -i output.ppm output.png   # FFmpeg
```

