#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"


struct Light {
    Light(const vec3 &p,const float &i) : position(p), intensity(i) {}
    vec3 position;
    float intensity;
};

struct Material
{
    // Material(const vec3 &color) : diffuse_color(color) {}
    Material(const vec2 &a, const vec3 &color, const float &spec) : albedo(a), diffuse_color(color), specular_exponent(spec) {}
    // Material(): diffuse_color() {}
    Material() : albedo{1, 0}, diffuse_color(), specular_exponent() {}
    
    vec2 albedo;
    vec3 diffuse_color;
    float specular_exponent;
};

struct Sphere
{
    vec3 center;
    float radius;
    Material material;

    Sphere(const vec3 &c, const float &r, const Material &m) : center(c), radius(r), material(m) {}

    bool ray_intersect(const vec3 &orig, const vec3 &dir, float &t0) const
    {
        vec3 L = center - orig;
        float tca = L * dir;
        float d2 = L * L - tca * tca;
        if (d2 > radius * radius)
            return false;
        float thc = sqrtf(radius * radius - d2);
        t0 = tca - thc;
        float t1 = tca + thc;
        if (t0 < 0)
            t0 = t1;
        if (t0 < 0)
            return false;
        return true;
    }
};

vec3 reflect(const vec3 &I, const vec3 &N){
    return I - N*2.f*(I*N); 
}

bool scene_intersect(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres, vec3 &hit, vec3 &N, Material &material) {
    float spheres_dist = std::numeric_limits<float>::max();
    for (size_t i=0; i<spheres.size(); i++) {
        float dist_i;
        if (spheres[i].ray_intersect(orig, dir, dist_i) && dist_i <spheres_dist) {
            spheres_dist = dist_i;
            hit = orig+dir * dist_i;
            N = (hit - spheres[i].center).normalize();
            material =spheres[i].material;
        }
    }
    return spheres_dist < 1000;
}

vec3 cast_ray(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres, const std::vector<Light> &lights)
{
    vec3 point, N;
    Material material;

    if (!scene_intersect(orig, dir, spheres, point, N, material)) {
        return vec3{0.2, 0.7, 0.8};
    }

    float diffuse_light_intensity = 0, specular_light_intentiy = 0;
    for (size_t i=0; i<lights.size(); i++) {
        vec3 light_dir = (lights[i].position - point).normalize();
        float light_distance = (lights[i].position - point).norm();

        vec3 shadow_orig = light_dir * N < 0? point - N * 1e-3 : point + N * 1e-3;
        vec3 shadow_pt, shadow_N;
        Material tmpmaterial;
        if (scene_intersect(shadow_orig, light_dir, spheres, shadow_pt, shadow_N, tmpmaterial) && (shadow_pt - shadow_orig).norm() < light_distance)
            continue;

        diffuse_light_intensity += lights[i].intensity * std::max(0.f, light_dir*N);
        specular_light_intentiy += powf(std::max(0.f, -reflect(-light_dir, N) * dir), material.specular_exponent) * lights[i].intensity;
    }

    return material.diffuse_color * diffuse_light_intensity * material.albedo[0] + 
                                        vec3{1, 1, 1}* specular_light_intentiy * material.albedo[1];
}

void render(const std::vector<Sphere> &spheres, const std::vector<Light> &lights)
{
    const int width = 1024;
    const int height = 768;
    const int fov = M_PI / 2.;
    // ! vector赋值用圆括号
    // std::vector<Vec3f> framebuffer(width*height);
    std::vector<vec3> framebuffer(width * height);

    for (size_t j = 0; j < height; j++)
    {
        for (size_t i = 0; i < width; i++)
        {
            // framebuffer[i + j * width] = vec3{j / float(height), i / float(width), 0};
            float x =  (2 * (i + 0.5) / (float)width  - 1) * tan(fov / 2.) * width / (float)height;
            float y = -(2 * (j + 0.5) / (float)height - 1) * tan(fov / 2.);
            vec3 dir = vec3{x, y, -1}.normalize();
            framebuffer[i + j * width] = cast_ray(vec3{0,0,0}, dir, spheres,  lights);
        }
    }

    std::ofstream ofs; // save the framebuffer to file
    ofs.open("../images/out.ppm");
    ofs << "P6\n"
        << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height * width; ++i)
    {
        vec3 &c = framebuffer[i];
        float max = std::max(c[0], std::max(c[1], c[2]));
        if (max > 1) c = c * (1. / max);

        for (size_t j = 0; j < 3; j++)
        {
            ofs << (char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][j])));
        }
    }
    ofs.close();
}

int main()
{
    Material      ivory(vec2{0.6, 0.3}, vec3{0.4, 0.4, 0.3}, 50.);
    Material red_rubber(vec2{0.9, 0.1}, vec3{0.3, 0.1, 0.1}, 10.);

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(vec3{ -3,    0, -16}, 2, ivory));
    spheres.push_back(Sphere(vec3{ -1, -1.5, -12}, 2, red_rubber));
    spheres.push_back(Sphere(vec3{1.5, -0.5, -18}, 3, red_rubber));
    spheres.push_back(Sphere(vec3{  7,    5, -18}, 4, ivory));

    std::vector<Light> lights;
    lights.push_back(Light(vec3{-20, 20, 20}, 1.5));
    lights.push_back(Light(vec3{ 30, 50,-25}, 1.8));
    lights.push_back(Light(vec3{ 30, 20, 30}, 1.7));
    
    render(spheres, lights);

    return 0;
}