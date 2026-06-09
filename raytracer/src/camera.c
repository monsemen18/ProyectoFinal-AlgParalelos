#include "raytracer.h"

/*
 * Traduce directamente la lógica de Camera.buildRay() del Java:
 *   - Calcula el plano focal según FOV y focalDistance
 *   - Mapea el píxel (px, py) + offset al espacio de cámara
 *   - Convierte de espacio cámara a espacio mundo con la matriz camToWorld
 */
Ray camera_build_ray(const Camera *cam,
                     int px, int py,
                     double offX, double offY)
{
    double alpha = cam->angleOfVision * M_PI / 180.0;
    double Wfp   = 2.0 * cam->focalDistance * tan(alpha / 2.0);
    double Hfp   = Wfp * cam->imageHeight / (double)cam->imageWidth;

    double xp = (px + offX) * Wfp / cam->imageWidth  - Wfp / 2.0;
    double yp = (py + offY) * Hfp / cam->imageHeight - Hfp / 2.0;

    /* Punto en el plano focal (espacio cámara) */
    Vec3 p_cam = vec3(xp, yp, 0.0);
    /* Origen del rayo en espacio cámara */
    Vec3 o_cam = vec3(0.0, 0.0, -cam->focalDistance);
    /* Dirección en espacio cámara */
    Vec3 d_cam = vec3_sub(p_cam, o_cam);

    /* Base ortonormal de la cámara  (igual que Java: u = dir × up, v = up, d = dir) */
    Vec3 u = vec3_norm(vec3_cross(cam->direction, cam->normalUp));
    Vec3 v = cam->normalUp;
    Vec3 d = cam->direction;

    Mat3 camToWorld = mat3(u, v, d);

    Vec3 d_world = vec3_norm(mat3_mul_vec(camToWorld, d_cam));
    return ray_make(cam->position, d_world);
}
