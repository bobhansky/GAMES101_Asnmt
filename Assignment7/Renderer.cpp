//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include "Scene.hpp"
#include "Renderer.hpp"


inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

constexpr bool MSAA = false;                   // MSAA switch;

const float EPSILON = 0.0001;       // original: 1e-5

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    float scale = tan(deg2rad(scene.fov * 0.5));        // half height : depth
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(278, 273, -800);
    int m = 0;

    // change the spp value to change sample ammount
    int spp = 16;
    std::cout << "SPP: " << spp << "\n";
    for (uint32_t j = 0; j < scene.height; ++j) {
        for (uint32_t i = 0; i < scene.width; ++i) {
            if (MSAA) {
                Vector3f res;

                float x = (2 * (i + 0.5) / (float)scene.width - 1) *
                    imageAspectRatio * scale;
                float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

                Vector3f dir = normalize(Vector3f(-x, y, 1));

                for (int k = 0; k < spp; k++) {
                    res += scene.castRay(Ray(eye_pos, dir), 0) / (5*spp);
                }


                for (float t = 0.25; t <= 0.75; t += 0.5) {
                    for (float r = 0.25; r <= 0.75; r += 0.5) {
                        // generate primary ray direction
                        float x = (2 * (i + t) / (float)scene.width - 1) *
                            imageAspectRatio * scale;
                        float y = (1 - 2 * (j + r) / (float)scene.height) * scale;

                        Vector3f dir = normalize(Vector3f(-x, y, 1));

                        for (int k = 0; k < spp; k++) {
                            res += scene.castRay(Ray(eye_pos, dir), 0) / (5 * spp);
                        }
                    }
                }
                framebuffer[m] = res;
                m++;
            }

            // none MSAA
            else {
                // generate primary ray direction
                float x = (2 * (i + 0.5) / (float)scene.width - 1) *
                    imageAspectRatio * scale;
                float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

                Vector3f dir = normalize(Vector3f(-x, y, 1));

                for (int k = 0; k < spp; k++) {
                    framebuffer[m] += scene.castRay(Ray(eye_pos, dir), 0) / spp;
                }
                m++;
            }
           
        }
        UpdateProgress(j / (float)scene.height);
    }
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);    
}
