#include "raytracer.h"

/* ─── Esfera ──────────────────────────────── */
Hit intersect_sphere(Ray ray, const Primitive *p) {
    Hit h = {0};
    Vec3 oc = vec3_sub(ray.origin, p->center);
    double b  = 2.0 * vec3_dot(ray.direction, oc);
    double c  = vec3_dot(oc, oc) - p->radius * p->radius;
    double disc = b*b - 4.0*c;
    if (disc < 0.0) return h;

    double sq = sqrt(disc);
    double t1 = (-b - sq) * 0.5;
    double t2 = (-b + sq) * 0.5;
    double t  = (t1 > 1e-6) ? t1 : (t2 > 1e-6 ? t2 : -1.0);
    if (t < 0.0) return h;

    h.valid  = 1;
    h.t      = t;
    h.point  = ray_at(ray, t);
    h.normal = vec3_norm(vec3_sub(h.point, p->center));
    return h;
}

/* ─── Plano infinito ──────────────────────── */
Hit intersect_plane(Ray ray, const Primitive *p) {
    Hit h = {0};
    double denom = vec3_dot(p->normal, ray.direction);
    if (fabs(denom) < 1e-8) return h;
    double t = vec3_dot(p->normal, vec3_sub(p->point, ray.origin)) / denom;
    if (t < 1e-6) return h;
    h.valid  = 1;
    h.t      = t;
    h.point  = ray_at(ray, t);
    h.normal = p->normal;
    return h;
}

/* ─── Caja AABB ───────────────────────────── */
Hit intersect_box(Ray ray, const Primitive *p) {
    Hit h = {0};
    Vec3 mn = p->box_pos;
    Vec3 mx = vec3_add(p->box_pos, vec3(p->width, p->height, p->depth));

    double tMin = -1e30, tMax = 1e30;
    int hitAxis = -1;

    double o[3] = { ray.origin.x,    ray.origin.y,    ray.origin.z    };
    double d[3] = { ray.direction.x, ray.direction.y, ray.direction.z };
    double mn_[3] = { mn.x, mn.y, mn.z };
    double mx_[3] = { mx.x, mx.y, mx.z };

    for (int i = 0; i < 3; i++) {
        if (fabs(d[i]) < 1e-8) {
            if (o[i] < mn_[i] || o[i] > mx_[i]) return h;
            continue;
        }
        double t0 = (mn_[i] - o[i]) / d[i];
        double t1 = (mx_[i] - o[i]) / d[i];
        if (t0 > t1) { double tmp=t0; t0=t1; t1=tmp; }
        if (t0 > tMin) { tMin = t0; hitAxis = i; }
        if (t1 < tMax) tMax = t1;
        if (tMin > tMax) return h;
    }
    if (tMin < 1e-6) return h;

    h.valid = 1;
    h.t     = tMin;
    h.point = ray_at(ray, tMin);

    /* Normal según el eje de impacto */
    Vec3 pt = h.point;
    double mn_v[3] = { mn.x, mn.y, mn.z };
    double p_arr[3] = { pt.x, pt.y, pt.z };
    (void)mx; /* solo se usa en el cálculo de tMax arriba */

    double nx[3] = {0,0,0};
    if (fabs(p_arr[hitAxis] - mn_v[hitAxis]) < 1e-5)
        nx[hitAxis] = -1.0;
    else
        nx[hitAxis] =  1.0;
    h.normal = vec3(nx[0], nx[1], nx[2]);
    return h;
}

/* ─── Impacto más cercano en la escena ───── */
Hit scene_find_closest_hit(const Scene *scene, Ray ray) {
    Hit closest = {0};

    for (int i = 0; i < scene->numPrimitives; i++) {
        const Primitive *p = &scene->primitives[i];
        Hit h;

        switch (p->type) {
            case PRIM_SPHERE: h = intersect_sphere(ray, p); break;
            case PRIM_PLANE:  h = intersect_plane (ray, p); break;
            case PRIM_BOX:    h = intersect_box   (ray, p); break;
            default: h.valid = 0; break;
        }

        if (h.valid && (!closest.valid || h.t < closest.t)) {
            closest          = h;
            closest.prim_index = i;
        }
    }
    return closest;
}
