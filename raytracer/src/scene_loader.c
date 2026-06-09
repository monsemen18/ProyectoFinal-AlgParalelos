#include "raytracer.h"
#include <cjson/cJSON.h>

/* ─── Helpers ─────────────────────────────── */
static Vec3 parse_vec3(cJSON *obj) {
    Vec3 v = {0,0,0};
    if (!obj) return v;
    cJSON *x = cJSON_GetObjectItemCaseSensitive(obj, "x");
    cJSON *y = cJSON_GetObjectItemCaseSensitive(obj, "y");
    cJSON *z = cJSON_GetObjectItemCaseSensitive(obj, "z");
    if (x) v.x = x->valuedouble;
    if (y) v.y = y->valuedouble;
    if (z) v.z = z->valuedouble;
    return v;
}

static ColorRGB parse_color(cJSON *obj) {
    ColorRGB c = {0,0,0};
    if (!obj) return c;
    cJSON *r = cJSON_GetObjectItemCaseSensitive(obj, "r");
    cJSON *g = cJSON_GetObjectItemCaseSensitive(obj, "g");
    cJSON *b = cJSON_GetObjectItemCaseSensitive(obj, "b");
    if (r) c.r = clampf((float)r->valuedouble);
    if (g) c.g = clampf((float)g->valuedouble);
    if (b) c.b = clampf((float)b->valuedouble);
    return c;
}

static const char *get_str(cJSON *obj, const char *key) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return (item && cJSON_IsString(item)) ? item->valuestring : "";
}

static double get_double(cJSON *obj, const char *key, double def) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return (item && cJSON_IsNumber(item)) ? item->valuedouble : def;
}

static int get_int(cJSON *obj, const char *key, int def) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return (item && cJSON_IsNumber(item)) ? (int)item->valuedouble : def;
}

/* ─── Cargar materiales ───────────────────── */
static void load_materials(cJSON *arr, Scene *s) {
    if (!arr) return;
    cJSON *m;
    cJSON_ArrayForEach(m, arr) {
        if (s->numMaterials >= MAX_MATERIALS) break;
        Material *mat = &s->materials[s->numMaterials++];
        strncpy(mat->id, get_str(m, "id"), MAX_NAME - 1);
        mat->color       = parse_color(cJSON_GetObjectItemCaseSensitive(m, "color"));
        mat->kd          = get_double(m, "diffuseCoefficient",  0.8);
        mat->ks          = get_double(m, "specularCoefficient", 0.2);
        mat->hardness    = get_int   (m, "specularHardness",    32);
        mat->reflectivity= get_double(m, "reflectivity",        0.0);
    }
}

/* ─── Cargar primitivas ───────────────────── */
static void load_primitives(cJSON *arr, Scene *s) {
    if (!arr) return;
    cJSON *p;
    cJSON_ArrayForEach(p, arr) {
        if (s->numPrimitives >= MAX_PRIMITIVES) break;
        Primitive *prim = &s->primitives[s->numPrimitives++];
        memset(prim, 0, sizeof(*prim));

        const char *type = get_str(p, "type");
        strncpy(prim->name,   get_str(p, "name"),     MAX_NAME - 1);
        strncpy(prim->mat_id, get_str(p, "material"), MAX_NAME - 1);
        prim->mat_index = -1;

        if (strcmp(type, "sphere") == 0) {
            prim->type   = PRIM_SPHERE;
            prim->center = parse_vec3(cJSON_GetObjectItemCaseSensitive(p, "center"));
            prim->radius = get_double(p, "radius", 1.0);

        } else if (strcmp(type, "plane") == 0) {
            prim->type   = PRIM_PLANE;
            prim->point  = parse_vec3(cJSON_GetObjectItemCaseSensitive(p, "point"));
            prim->normal = vec3_norm(parse_vec3(cJSON_GetObjectItemCaseSensitive(p, "normal")));

        } else if (strcmp(type, "box") == 0) {
            prim->type    = PRIM_BOX;
            prim->box_pos = parse_vec3(cJSON_GetObjectItemCaseSensitive(p, "position"));
            prim->width   = get_double(p, "width",  1.0);
            prim->height  = get_double(p, "height", 1.0);
            prim->depth   = get_double(p, "depth",  1.0);
        }
    }
}

/* ─── Cargar luces ────────────────────────── */
static void load_lights(cJSON *arr, Scene *s) {
    if (!arr) return;
    cJSON *l;
    cJSON_ArrayForEach(l, arr) {
        if (s->numLights >= MAX_LIGHTS) break;
        Light *light = &s->lights[s->numLights++];
        memset(light, 0, sizeof(*light));

        const char *type = get_str(l, "type");
        light->color     = parse_color(cJSON_GetObjectItemCaseSensitive(l, "color"));
        light->intensity = get_double(l, "intensity", 1.0);

        if (strcmp(type, "pointLight") == 0) {
            light->type     = LIGHT_POINT;
            light->position = parse_vec3(cJSON_GetObjectItemCaseSensitive(l, "position"));

        } else if (strcmp(type, "directionalLight") == 0) {
            light->type      = LIGHT_DIRECTIONAL;
            light->direction = vec3_norm(parse_vec3(cJSON_GetObjectItemCaseSensitive(l, "direction")));

        } else if (strcmp(type, "surfaceLight") == 0) {
            light->type      = LIGHT_SURFACE;
            light->position  = parse_vec3(cJSON_GetObjectItemCaseSensitive(l, "position"));
            light->direction = vec3_norm(parse_vec3(cJSON_GetObjectItemCaseSensitive(l, "direction")));
            light->size      = get_double(l, "size",    1.0);
            light->samples   = get_int   (l, "samples", 16);
        }
    }
}

/* ─── Cargar cámara ───────────────────────── */
static void load_camera(cJSON *obj, Camera *cam) {
    if (!obj) return;
    cam->imageWidth     = get_int   (obj, "imageWidth",     800);
    cam->imageHeight    = get_int   (obj, "imageHeight",    600);
    cam->position       = parse_vec3(cJSON_GetObjectItemCaseSensitive(obj, "position"));
    cam->direction      = vec3_norm (parse_vec3(cJSON_GetObjectItemCaseSensitive(obj, "direction")));
    cam->normalUp       = vec3_norm (parse_vec3(cJSON_GetObjectItemCaseSensitive(obj, "normalUp")));
    cam->angleOfVision  = get_double(obj, "angleOfVision", 60.0);
    cam->focalDistance  = get_double(obj, "focalDistance",  1.0);
}

/* ─── Vincular materiales ─────────────────── */
static void link_materials(Scene *s) {
    for (int i = 0; i < s->numPrimitives; i++) {
        Primitive *p = &s->primitives[i];
        for (int j = 0; j < s->numMaterials; j++) {
            if (strcmp(p->mat_id, s->materials[j].id) == 0) {
                p->mat_index = j;
                break;
            }
        }
        if (p->mat_index < 0) {
            fprintf(stderr, "Warning: material '%s' no encontrado para '%s'\n",
                    p->mat_id, p->name);
        }
    }
}

/* ─── Leer archivo completo ───────────────── */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = (char *)malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t nread = fread(buf, 1, sz, f);
    buf[nread] = '\0';
    fclose(f);
    return buf;
}

/* ─── Entrada pública ─────────────────────── */
int scene_load_json(const char *path, Scene *s) {
    memset(s, 0, sizeof(*s));

    char *text = NULL;
    cJSON *root = NULL;

    if (path) {
        text = read_file(path);
        if (!text) return -1;
        root = cJSON_Parse(text);
        free(text);
    } else {
        /* Leer desde stdin */
        size_t cap = 4096, len = 0;
        text = (char *)malloc(cap);
        int c;
        while ((c = fgetc(stdin)) != EOF) {
            if (len + 1 >= cap) { cap *= 2; text = (char *)realloc(text, cap); }
            text[len++] = (char)c;
        }
        text[len] = '\0';
        root = cJSON_Parse(text);
        free(text);
    }

    if (!root) {
        fprintf(stderr, "Error al parsear JSON: %s\n", cJSON_GetErrorPtr());
        return -1;
    }

    s->samplesPerPixel       = get_int   (root, "samplesPerPixel",       4);
    s->rayMaxBounces         = get_int   (root, "rayMaxBounces",         4);
    s->ambientLightIntensity = get_double(root, "ambientLightIntensity", 0.1);
    s->background            = parse_color(cJSON_GetObjectItemCaseSensitive(root, "background"));

    load_camera    (cJSON_GetObjectItemCaseSensitive(root, "camera"),     &s->camera);
    load_materials (cJSON_GetObjectItemCaseSensitive(root, "materials"),   s);
    load_primitives(cJSON_GetObjectItemCaseSensitive(root, "primitives"),  s);
    load_lights    (cJSON_GetObjectItemCaseSensitive(root, "lights"),      s);

    link_materials(s);

    cJSON_Delete(root);
    return 0;
}
