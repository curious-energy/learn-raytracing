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
    Material(const float &r, const vec4 &a, const vec3 &color, const float &spec)
        : refractive_index(r), albedo(a), diffuse_color(color), specular_exponent(spec) {}
    // Material(): diffuse_color() {}
    Material() : refractive_index(1), albedo(vec4{1, 0, 0, 0}), diffuse_color(vec3{0,0,0}), specular_exponent(0) {}
    
    float refractive_index;
    vec4 albedo;
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
        if (t0 < 1e-3)
            t0 = t1;
        if (t0 < 1e-3)
            return false;
        return true;
    }
};

vec3
refract(const vec3 &I, const vec3 &N, const float &refractive_index) {
    float cosi = - std::max(-1.f, std::min(1.f, I*N));
    float etai = 1, etat = refractive_index;
    vec3 n = N;
    if(cosi < 0) {
        cosi = -cosi;
        std::swap(etai, etat);
        n = -N;
    }
    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? vec3{0,0,0} : I * eta + n * (eta * cosi - sqrtf(k));
}

vec3
reflect(const vec3 &I, const vec3 &N){
    return I - N*2.f*(I*N); 
}

bool
scene_intersect(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres, vec3 &hit, vec3 &N, Material &material) {
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
    // return spheres_dist < 1000;
    float checkerboard_dist = std::numeric_limits<float>::max();
    if (fabs(dir.y)>1e-3) {
        float d = - ( orig.y + 4) / dir.y; // checkerboard在y=-4平面上
        vec3 pt = orig + dir * d;
        if (d > 1e-3 && fabs(pt.x)<10 && pt.z<-10 && pt.z>-30 && d<spheres_dist) {
            checkerboard_dist = d;
            hit =pt;
            N = vec3{0, 1, 0};
            material.diffuse_color = (int(.5*hit.x +1000) + int(.5*hit.z)) & 1 ? vec3{.3,.3,.3} : vec3{.3,.2,.1};
            // material.diffuse_color = material.diffuse_color * .3;
        }
    }
    return std::min(spheres_dist, checkerboard_dist) < 1000;
}

vec3
cast_ray(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres, const std::vector<Light> &lights, size_t depth=0)
{
    vec3 point, N;
    Material material;

    if ( depth > 4 || !scene_intersect(orig, dir, spheres, point, N, material)) {
        return vec3{0.2, 0.7, 0.8};
    }

    // reflect
    vec3 reflect_dir = reflect(dir, N).normalize();
    vec3 refract_dir = refract(dir, N, material.refractive_index).normalize();
    vec3 reflect_orig = reflect_dir * N <0 ? point - N*1e-3 : point + N * 1e-3;
    vec3 refract_orig = refract_dir * N <0 ? point - N*1e-3 : point + N * 1e-3;
    vec3 reflect_color = cast_ray(reflect_orig, reflect_dir, spheres, lights, depth+1);
    vec3 refract_color = cast_ray(refract_orig, refract_dir, spheres, lights, depth+1);

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

    return material.diffuse_color * diffuse_light_intensity * material.albedo[0]
                + vec3{1.0, 1.0, 1.0} * specular_light_intentiy * material.albedo[1]
                + reflect_color * material.albedo[2]
                + refract_color * material.albedo[3];
}

void
render(const std::vector<Sphere> &spheres, const std::vector<Light> &lights)
{
    const int width = 1024;
    const int height = 768;
    const int fov = M_PI / 3.;
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
    Material      ivory(1.0, vec4{0.6, 0.3, 0.1, 0.0}, vec3{0.4, 0.4, 0.3}, 50.);
    Material      glass(1.5, vec4{0.0, 0.5, 0.1, 0.8}, vec3{0.6, 0.7, 0.8}, 125.);
    Material red_rubber(1.0, vec4{0.9, 0.1, 0.0, 0.0}, vec3{0.3, 0.1, 0.1}, 10.);
    Material     mirror(1.0, vec4{0.0, 10.0, 0.8, 0.0}, vec3{1.0, 1.0, 1.0}, 1425.);

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(vec3{ -3,    0, -16}, 2, ivory));
    spheres.push_back(Sphere(vec3{ -1.0, -1.5, -12}, 2, glass));
    spheres.push_back(Sphere(vec3{1.5, -0.5, -18}, 3, red_rubber));
    spheres.push_back(Sphere(vec3{  7,    5, -18}, 4, mirror));

    std::vector<Light> lights;
    lights.push_back(Light(vec3{-20, 20, 20}, 1.5));
    lights.push_back(Light(vec3{ 30, 50,-25}, 1.8));
    lights.push_back(Light(vec3{ 30, 20, 30}, 1.7));
    
    render(spheres, lights);

    return 0;
}