#include "raytracer.h"

/* ─── Forward declarations internas ─────────── */
static ColorRGB shade_internal(Ray ray, const Scene *scene, int bounces);

/* ─── Reflejo de vector ──────────────────────── */
static Vec3 reflect_vec(Vec3 incident, Vec3 normal) {
    Vec3 n = vec3_norm(normal);
    return vec3_norm(vec3_sub(incident,
                              vec3_scale(n, 2.0 * vec3_dot(incident, n))));
}

/* ─── Sombra dura ────────────────────────────── */
static float hard_shadow(Vec3 P, Vec3 L, double lightDist,
                          const Scene *scene) {
    Vec3 orig = vec3_add(P, vec3_scale(L, EPSILON));
    Ray sray  = ray_make(orig, L);

    for (int i = 0; i < scene->numPrimitives; i++) {
        const Primitive *prim = &scene->primitives[i];
        Hit h;
        switch (prim->type) {
            case PRIM_SPHERE: h = intersect_sphere(sray, prim); break;
            case PRIM_PLANE:  h = intersect_plane (sray, prim); break;
            case PRIM_BOX:    h = intersect_box   (sray, prim); break;
            default: h.valid = 0; break;
        }
        if (h.valid && h.t >= EPSILON && h.t < lightDist)
            return 0.0f;
    }
    return 1.0f;
}

/* ─── Sombra suave (luz de área) ─────────────── */
static float soft_shadow(Vec3 P, const Light *light, const Scene *scene) {
    double *pts;
    int     count;
    sampling_grid(light->size, light->samples, &pts, &count);

    /* Base ortonormal de la luz */
    Vec3 d  = vec3_norm(light->direction);
    Vec3 up = vec3(0, 1, 0);
    if (fabs(vec3_dot(d, up)) > 0.99) up = vec3(1, 0, 0);
    Vec3 u  = vec3_norm(vec3_cross(d, up));
    Vec3 v  = vec3_norm(vec3_cross(d, u));
    Mat3 ltw = mat3(u, v, d);

    int unoccluded = 0;
    double half = light->size / 2.0;

    for (int k = 0; k < count; k++) {
        double lx = pts[2*k]   - half;
        double ly = pts[2*k+1] - half;
        Vec3 localSample = vec3(lx, ly, 0);
        Vec3 worldSample = vec3_add(light->position,
                                    mat3_mul_vec(ltw, localSample));
        Vec3 dir  = vec3_sub(worldSample, P);
        double ld = vec3_len(dir);
        Vec3 L    = vec3_scale(dir, 1.0/ld);
        unoccluded += (int)hard_shadow(P, L, ld, scene);
    }
    free(pts);
    return (float)unoccluded / (float)count;
}

/* ─── Iluminación difusa ─────────────────────── */
static Vec3 diffuse_contrib(const Light *light, const Material *mat,
                             Vec3 N, Vec3 L) {
    Vec3   lc  = color_to_vec3(light->color);
    Vec3   lci = vec3_scale(lc, light->intensity);
    double nl  = fmax(0.0, vec3_dot(N, L));
    return vec3_scale(lci, mat->kd * nl);
}

/* ─── Iluminación especular ──────────────────── */
static Vec3 specular_contrib(const Light *light, const Material *mat,
                              Vec3 N, Vec3 L, Vec3 V) {
    Vec3   lc  = color_to_vec3(light->color);
    Vec3   lci = vec3_scale(lc, light->intensity);
    Vec3   R   = reflect_vec(vec3_scale(L, -1.0), N);
    double vr  = fmax(0.0, vec3_dot(V, R));
    return vec3_scale(lci, mat->ks * pow(vr, mat->hardness));
}

/* ─── Obtener dirección y distancia a una luz ── */
static void light_dir_dist(const Light *light, Vec3 P,
                            Vec3 *L_out, double *dist_out) {
    if (light->type == LIGHT_DIRECTIONAL) {
        *L_out    = vec3_norm(vec3_scale(light->direction, -1.0));
        *dist_out = 1e30;
    } else {
        Vec3 diff = vec3_sub(light->position, P);
        *dist_out = vec3_len(diff);
        *L_out    = vec3_scale(diff, 1.0 / (*dist_out));
    }
}

/* ─── Phong completo (con rayo) ──────────────── */
static Vec3 compute_phong(const Hit *hit, Ray ray, const Scene *scene,
                           const Material *mat) {
    Vec3 P = hit->point;
    Vec3 N = vec3_norm(hit->normal);
    Vec3 V = vec3_norm(vec3_scale(ray.direction, -1.0));

    Vec3 lighting = vec3(0,0,0);

    for (int li = 0; li < scene->numLights; li++) {
        const Light *light = &scene->lights[li];

        Vec3   L;
        double lightDist;
        light_dir_dist(light, P, &L, &lightDist);

        float shadow = (light->type == LIGHT_SURFACE)
                       ? soft_shadow(P, light, scene)
                       : hard_shadow(P, L, lightDist, scene);

        Vec3 mColor = color_to_vec3(mat->color);

        /* Difusa × color material × sombra */
        Vec3 Id = vec3_mul(
                     vec3_scale(diffuse_contrib(light, mat, N, L), shadow),
                     mColor);

        /* Especular × sombra */
        Vec3 Is = vec3_scale(specular_contrib(light, mat, N, L, V), shadow);

        lighting = vec3_add(lighting, vec3_add(Id, Is));
    }

    /* Luz ambiental */
    Vec3 ambient = vec3_scale(color_to_vec3(mat->color),
                              scene->ambientLightIntensity);
    return vec3_add(lighting, ambient);
}

/* ─── shade_internal ─────────────────────────── */
static ColorRGB shade_internal(Ray ray, const Scene *scene, int bounces) {
    Hit hit = scene_find_closest_hit(scene, ray);
    if (!hit.valid)
        return scene->background;

    const Primitive *prim = &scene->primitives[hit.prim_index];
    const Material  *mat  = (prim->mat_index >= 0)
                            ? &scene->materials[prim->mat_index]
                            : &scene->materials[0];

    Vec3 local = compute_phong(&hit, ray, scene, mat);

    if (mat->reflectivity > 0.0 && bounces > 0) {
        Vec3 I = ray.direction;
        Vec3 N = hit.normal;
        Vec3 R = reflect_vec(I, N);
        Vec3 reflOrig = vec3_add(hit.point, vec3_scale(R, EPSILON));
        Ray  reflRay  = ray_make(reflOrig, R);

        ColorRGB reflColor = shade_internal(reflRay, scene, bounces - 1);
        Vec3     reflVec   = color_to_vec3(reflColor);

        double r = mat->reflectivity;
        Vec3 blended = vec3_add(vec3_scale(local, 1.0 - r),
                                vec3_scale(reflVec, r));
        return vec3_to_color(blended);
    }

    return vec3_to_color(local);
}

/* ─── Entrada pública ────────────────────────── */
ColorRGB shade(Ray ray, const Scene *scene, int bounces) {
    return shade_internal(ray, scene, bounces);
}
