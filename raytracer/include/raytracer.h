#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ─────────────────────────────────────────────
   CONSTANTES
───────────────────────────────────────────── */
#define EPSILON       1e-4
#define MAX_PRIMITIVES 256
#define MAX_LIGHTS     64
#define MAX_MATERIALS  64
#define MAX_NAME       64
#define MAX_BOUNCES    16

/* ─────────────────────────────────────────────
   VECTOR 3D
───────────────────────────────────────────── */
typedef struct {
    double x, y, z;
} Vec3;

static inline Vec3  vec3(double x, double y, double z){ return (Vec3){x,y,z}; }
static inline Vec3  vec3_add (Vec3 a, Vec3 b){ return (Vec3){a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline Vec3  vec3_sub (Vec3 a, Vec3 b){ return (Vec3){a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline Vec3  vec3_scale(Vec3 a, double s){ return (Vec3){a.x*s, a.y*s, a.z*s}; }
static inline Vec3  vec3_mul (Vec3 a, Vec3 b){ return (Vec3){a.x*b.x, a.y*b.y, a.z*b.z}; }
static inline double vec3_dot(Vec3 a, Vec3 b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline Vec3  vec3_cross(Vec3 a, Vec3 b){
    return (Vec3){ a.y*b.z - a.z*b.y,
                   a.z*b.x - a.x*b.z,
                   a.x*b.y - a.y*b.x };
}
static inline double vec3_len(Vec3 a){ return sqrt(vec3_dot(a,a)); }
static inline Vec3 vec3_norm(Vec3 a){
    double l = vec3_len(a);
    if (l == 0.0) return a;
    return vec3_scale(a, 1.0/l);
}

/* ─────────────────────────────────────────────
   COLOR RGB  (floats [0,1])
───────────────────────────────────────────── */
typedef struct { float r, g, b; } ColorRGB;

static inline float clampf(float x){ return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }
static inline ColorRGB color(float r, float g, float b){
    return (ColorRGB){clampf(r), clampf(g), clampf(b)};
}
static inline ColorRGB color_add(ColorRGB a, ColorRGB b){
    return color(a.r+b.r, a.g+b.g, a.b+b.b);
}
static inline ColorRGB color_scale(ColorRGB c, float s){
    return color(c.r*s, c.g*s, c.b*s);
}
/* ColorRGB <-> Vec3 */
static inline Vec3    color_to_vec3(ColorRGB c){ return vec3(c.r, c.g, c.b); }
static inline ColorRGB vec3_to_color(Vec3 v)   { return color((float)v.x,(float)v.y,(float)v.z); }

/* ─────────────────────────────────────────────
   MATRIZ 3x3  (columnas)
───────────────────────────────────────────── */
typedef struct { Vec3 col[3]; } Mat3;

static inline Mat3 mat3(Vec3 u, Vec3 v, Vec3 d){ return (Mat3){{u,v,d}}; }
static inline Vec3 mat3_mul_vec(Mat3 m, Vec3 v){
    return vec3_add(vec3_add(vec3_scale(m.col[0], v.x),
                             vec3_scale(m.col[1], v.y)),
                             vec3_scale(m.col[2], v.z));
}

/* ─────────────────────────────────────────────
   RAYO
───────────────────────────────────────────── */
typedef struct {
    Vec3 origin;
    Vec3 direction; /* normalizado */
} Ray;

static inline Ray ray_make(Vec3 o, Vec3 d){
    return (Ray){o, vec3_norm(d)};
}
static inline Vec3 ray_at(Ray r, double t){
    return vec3_add(r.origin, vec3_scale(r.direction, t));
}

/* ─────────────────────────────────────────────
   MATERIAL
───────────────────────────────────────────── */
typedef struct {
    char     id[MAX_NAME];
    ColorRGB color;
    double   kd;                /* diffuseCoefficient  */
    double   ks;                /* specularCoefficient */
    int      hardness;          /* specularHardness    */
    double   reflectivity;
} Material;

/* ─────────────────────────────────────────────
   PRIMITIVAS
───────────────────────────────────────────── */
typedef enum { PRIM_SPHERE, PRIM_PLANE, PRIM_BOX } PrimType;

typedef struct {
    PrimType type;
    char     name[MAX_NAME];
    char     mat_id[MAX_NAME];
    int      mat_index;          /* resuelto en linkMaterials */

    /* Sphere */
    Vec3   center;
    double radius;

    /* Plane */
    Vec3 point;
    Vec3 normal;

    /* Box */
    Vec3   box_pos;
    double width, height, depth;
} Primitive;

/* ─────────────────────────────────────────────
   LUCES
───────────────────────────────────────────── */
typedef enum { LIGHT_POINT, LIGHT_DIRECTIONAL, LIGHT_SURFACE } LightType;

typedef struct {
    LightType type;
    ColorRGB  color;
    double    intensity;

    Vec3      position;   /* point / surface */
    Vec3      direction;  /* directional / surface */
    double    size;       /* surface */
    int       samples;    /* surface */
} Light;

/* ─────────────────────────────────────────────
   CÁMARA
───────────────────────────────────────────── */
typedef struct {
    int    imageWidth, imageHeight;
    Vec3   position;
    Vec3   direction;
    Vec3   normalUp;
    double angleOfVision; /* grados */
    double focalDistance;
} Camera;

/* ─────────────────────────────────────────────
   ESCENA
───────────────────────────────────────────── */
typedef struct {
    int    samplesPerPixel;
    int    rayMaxBounces;
    double ambientLightIntensity;

    ColorRGB background;
    Camera   camera;

    Material   materials[MAX_MATERIALS];
    int        numMaterials;

    Primitive  primitives[MAX_PRIMITIVES];
    int        numPrimitives;

    Light      lights[MAX_LIGHTS];
    int        numLights;
} Scene;

/* ─────────────────────────────────────────────
   HIT  (resultado de intersección)
───────────────────────────────────────────── */
typedef struct {
    double    t;
    Vec3      point;
    Vec3      normal;
    int       prim_index; /* índice en scene->primitives */
    int       valid;
} Hit;

/* ─────────────────────────────────────────────
   IMAGEN
───────────────────────────────────────────── */
typedef struct {
    int      width, height;
    ColorRGB *pixels;           /* row-major: pixels[y*width + x] */
} Image;

Image *image_create(int w, int h);
void   image_destroy(Image *img);
void   image_set_pixel(Image *img, int x, int y, ColorRGB c);
ColorRGB image_get_pixel(Image *img, int x, int y);
int    image_write_ppm(Image *img, const char *path);

/* ─────────────────────────────────────────────
   API pública
───────────────────────────────────────────── */
/* scene_loader.c */
int scene_load_json(const char *path, Scene *scene);

/* intersect.c */
Hit  intersect_sphere(Ray ray, const Primitive *p);
Hit  intersect_plane (Ray ray, const Primitive *p);
Hit  intersect_box   (Ray ray, const Primitive *p);
Hit  scene_find_closest_hit(const Scene *scene, Ray ray);

/* camera.c */
Ray  camera_build_ray(const Camera *cam, int px, int py,
                      double offX, double offY);

/* shader.c */
ColorRGB shade(Ray ray, const Scene *scene, int bounces);

/* raytracer.c */
Image *render_serial  (const Scene *scene);
Image *render_parallel(const Scene *scene, int numThreads);

/* sampling.c */
void sampling_grid(double size, int samples, double **pts, int *count);

#endif /* RAYTRACER_H */
