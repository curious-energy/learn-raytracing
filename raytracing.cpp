#include <limits>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"

struct Material
{
    Material(const vec3 &color) : diffuse_color(color) {}
    Material(): diffuse_color() {}
    vec3 diffuse_color;
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

vec3 cast_ray(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres)
{
    vec3 point, N;
    Material matetial;

    if (!scene_intersect(orig, dir, spheres, point, N, matetial)) {
        return vec3{0.2, 0.7, 0.8};
    }
    return matetial.diffuse_color;
}

void render(const std::vector<Sphere> &spheres)
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
            framebuffer[i + j * width] = cast_ray(vec3{0,0,0}, dir, spheres);
        }
    }

    std::ofstream ofs; // save the framebuffer to file
    ofs.open("../images/out.ppm");
    ofs << "P6\n"
        << width << " " << height << "\n255\n";
    for (size_t i = 0; i < height * width; ++i)
    {
        for (size_t j = 0; j < 3; j++)
        {
            ofs << (char)(255 * std::max(0.f, std::min(1.f, framebuffer[i][j])));
        }
    }
    ofs.close();
}

int main()
{
    Material      ivory(vec3{0.4, 0.4, 0.3});
    Material red_rubber(vec3{0.3, 0.1, 0.1});

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(vec3{ -3,    0, -16}, 2, ivory));
    spheres.push_back(Sphere(vec3{ -1, -1.5, -12}, 2, red_rubber));
    spheres.push_back(Sphere(vec3{1.5, -0.5, -18}, 3, red_rubber));
    spheres.push_back(Sphere(vec3{  7,    5, -18}, 4, ivory));
    
    render(spheres);

    return 0;
}