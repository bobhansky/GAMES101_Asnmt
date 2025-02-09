﻿#include <iostream>
#include <opencv2/opencv.hpp>
#include<cmath>

#include "global.hpp"
#include "rasterizer.hpp"
#include "Triangle.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "OBJ_Loader.h"

#define LERP true

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1,0,0,-eye_pos[0],
                 0,1,0,-eye_pos[1],
                 0,0,1,-eye_pos[2],
                 0,0,0,1;

    view = translate*view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float angle)
{
    Eigen::Matrix4f rotation;
    angle = angle * MY_PI / 180.f;
    // rotation around y axis
    rotation << cos(angle), 0, sin(angle), 0,
                0, 1, 0, 0,
                -sin(angle), 0, cos(angle), 0,
                0, 0, 0, 1;

    Eigen::Matrix4f scale;
    scale << 2.5, 0, 0, 0,          // original 2.5 on each axis
              0, 2.5, 0, 0,
              0, 0, 2.5, 0,
              0, 0, 0, 1;

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1;

    return translate * rotation * scale;
}

// my enhancement part   10/11/2022
Eigen::Matrix4f get_rotation_matrix(float rota_angle, Eigen::Vector3f& axis) {
    // convert degrees to radians
    rota_angle = (rota_angle / 180) * MY_PI;

    Eigen::Vector3f normAxis = axis.normalized();
    Eigen::Matrix4f res = Eigen::Matrix4f::Zero();
    res(3, 3) = 1;  // 0 0 0 1 last row

    Eigen::Matrix3f identity = Eigen::Matrix3f::Identity();
    Eigen::Matrix3f N;
    N << 0, -normAxis.z(), normAxis.y(),
        normAxis.z(), 0, -normAxis.x(),
        -normAxis.y(), normAxis.x(), 0;

    N = std::cos(rota_angle) * identity + (1 - std::cos(rota_angle)) * normAxis * normAxis.transpose()
        + sin(rota_angle) * N;

    // block   https://eigen.tuxfamily.org/dox/group__TutorialBlockOperations.html
    // starting at (i,j) Block of size (p,q),   matrix.block(i,j,p,q);
    res.block(0, 0, 2, 2) = N.block(0, 0, 2, 2);

    Eigen::Matrix4f scale;
    scale << 2.5, 0, 0, 0,          // original 2.5 on each axis
        0, 2.5, 0, 0,
        0, 0, 2.5, 0,
        0, 0, 0, 1;

    res = scale * res;
    return res;
}


Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio, float zNear, float zFar)
{
    // TODO: Copy-paste your implementation from the previous assignment.

    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();


    // perspective projection to orthogonal projection
    Eigen::Matrix4f per_to_ortho;
    per_to_ortho << zNear, 0, 0, 0,
        0, zNear, 0, 0,
        0, 0, zNear + zFar, -zNear * zFar,
        0, 0, 1, 0;


    // orthogonal projection part
    // fov is in degrees

    float t = std::tan((eye_fov / 2) * MY_PI / 180) * -zNear;    // tan() takes radians
    float b = -t;
    float r = aspect_ratio * t;
    float l = -r;

    Eigen::Matrix4f orth_translate;
    orth_translate << 1, 0, 0, -(r + l) / 2,
        0, 1, 0, -(t + b) / 2,
        0, 0, 1, -(zNear + zFar) / 2,
        0, 0, 0, 1;

    Eigen::Matrix4f orth_scale;
    orth_scale << 2 / (r - l), 0, 0, 0,
        0, 2 / (t - b), 0, 0,
        0, 0, 2 / (zNear - zFar), 0,
        0, 0, 0, 1;

    Eigen::Matrix4f ortho_proj = orth_scale * orth_translate;

    projection = ortho_proj * per_to_ortho * projection;


    return projection;
}


Eigen::Vector3f vertex_shader(const vertex_shader_payload& payload)
{
    return payload.position;
}

Eigen::Vector3f normal_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f return_color = (payload.normal.head<3>().normalized() + Eigen::Vector3f(1.0f, 1.0f, 1.0f)) / 2.f;
    Eigen::Vector3f result;
    result << return_color.x() * 255, return_color.y() * 255, return_color.z() * 255;
    return result;
}

static Eigen::Vector3f reflect(const Eigen::Vector3f& vec, const Eigen::Vector3f& axis)
{
    auto costheta = vec.dot(axis);
    return (2 * costheta * axis - vec).normalized();
}

struct light
{
    Eigen::Vector3f position;
    Eigen::Vector3f intensity;
};



Eigen::Vector3f phong_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);      // ambient coefficient
    Eigen::Vector3f kd = payload.color;                 // diffuse coefficient,   color
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);       // Ks 一般是一个白色的镜面反射系数

    auto l1 = light{ {20, 20, 20}, {500, 500, 500} };       // position and intensity
    auto l2 = light{ {-20, 20, 0}, {500, 500, 500} };

    std::vector<light> lights = { l1, l2 };
    Eigen::Vector3f amb_light_intensity{ 10, 10, 10 };
    Eigen::Vector3f eye_pos{ 0, 0, 10 };

    float p = 150;

    Eigen::Vector3f color = payload.color;      // not used, so what is it?
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = { 0, 0, 0 };
    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *result_color* object.

        Eigen::Vector3f l = (light.position - point).normalized();      // light direction
        Eigen::Vector3f v = ( - point).normalized();     // view direction     0 - point 
        Eigen::Vector3f h = (v + l) / (v + l).norm();           // bisector 半程向量

        float r_2 = (light.position - point).dot(light.position - point);     // or r_2 = (l-point).squaredNorm()

        // cwiseProduct可以实现两个向量对应元素一一相乘。   element-wise    https://www.bbsmax.com/A/l1dyONwGJe/
        auto diffuse = kd.cwiseProduct((light.intensity / r_2) * (MAX(0, normal.dot(l))));
        auto specular = ks.cwiseProduct((light.intensity / r_2) * (MAX(0, std::pow(normal.dot(h), p))));
        auto ambient = ka.cwiseProduct(amb_light_intensity);

        result_color = result_color + ambient + diffuse + specular;  // need to += or write in this way
    }

    return result_color * 255.f;  // this line is provided
}



Eigen::Vector3f texture_fragment_shader(const fragment_shader_payload& payload)
    // 要更换纹理路径!!!
{
    Eigen::Vector3f return_color = {0, 0, 0};
    if (payload.texture)
    {   
        // TODO: Get the texture value at the texture coordinates of the current fragment
        if (LERP) {
            return_color = payload.texture->getColorBilinear(payload.tex_coords.x(), payload.tex_coords.y());
        }
        else return_color = payload.texture->getColor(payload.tex_coords.x(), payload.tex_coords.y());
    }

    Eigen::Vector3f texture_color;
    texture_color << return_color.x(), return_color.y(), return_color.z();

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = texture_color / 255.f;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = texture_color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};

    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *res..ult_color* object.
        Eigen::Vector3f l = (light.position - point).normalized();      // light direction
        Eigen::Vector3f v = (eye_pos - point).normalized();     // view direction
        Eigen::Vector3f h = (v + l) / (v + l).norm();           // bisector 半程向量

        float r_2 = (light.position - point).dot(light.position - point);     // or r_2 = (l-point).squaredNorm()

        auto diffuse = kd.cwiseProduct(light.intensity / r_2 * MAX(0, normal.dot(l)));
        auto specular = ks.cwiseProduct(light.intensity / r_2 * MAX(0, std::pow(normal.dot(h), p)));
        auto ambient = ka.cwiseProduct(amb_light_intensity);

        result_color = result_color + ambient + diffuse + specular;
    }

    return result_color * 255.f;
}



Eigen::Vector3f bump_fragment_shader(const fragment_shader_payload& payload)
{
    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color; 
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;


    float kh = 0.2, kn = 0.1;

    // TODO: Implement bump mapping here
    // Let n = normal = (x, y, z)
    Eigen::Vector3f n(payload.normal.x(), payload.normal.y(), payload.normal.z());

    // Vector t = (x*y/sqrt(x*x+z*z),sqrt(x*x+z*z),z*y/sqrt(x*x+z*z))
    Eigen::Vector3f t(n.x() * n.y() / sqrt(n.x() * n.x() + n.z() * n.z()), sqrt(n.x() * n.x() + n.z() * n.z()), 
        n.z() * n.y() / sqrt(n.x() * n.x() + n.z() * n.z()));

    // Vector b = n cross product t
    auto b = n.cross(t);

    // Matrix TBN = [t b n]
    Eigen::Matrix3f TBN;
    TBN << t.x(), b.x(), n.x(),
        t.y(), b.y(), n.y(),
        t.z(), b.z(), n.z();

    // dU = kh * kn * (h(u+1/w,v)-h(u,v))
    // dV = kh * kn * (h(u,v+1/h)-h(u,v))
    float u = payload.tex_coords.x();
    float v = payload.tex_coords.y();
    float h = payload.texture->height;
    float w = payload.texture->width;

    // cant understand   I just copy
    float dU = kh * kn * (payload.texture->getColor(u + 1.0f / w, v).norm() - payload.texture->getColor(u, v).norm());
    float dV = kh * kn * (payload.texture->getColor(u, v + 1.0f / h).norm() - payload.texture->getColor(u, v).norm());

    // Vector ln = (-dU, -dV, 1)
    Eigen::Vector3f ln(-dU, -dV, 1);

    // Normal n = normalize(TBN * ln)
    normal = (TBN * ln).normalized();


    Eigen::Vector3f result_color = {0, 0, 0};
    result_color = normal;

    return result_color * 255.f;
}


Eigen::Vector3f displacement_fragment_shader(const fragment_shader_payload& payload)
{

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{ {20, 20, 20}, {500, 500, 500} };
    auto l2 = light{ {-20, 20, 0}, {500, 500, 500} };

    std::vector<light> lights = { l1, l2 };
    Eigen::Vector3f amb_light_intensity{ 10, 10, 10 };
    Eigen::Vector3f eye_pos{ 0, 0, 10 };

    float p = 150;

    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    float kh = 0.2, kn = 0.1;

    // TODO: Implement displacement mapping here
    // Let n = normal = (x, y, z)
    float x = normal.x(), y = normal.y(), z = normal.z();

    // Vector t = (x*y/sqrt(x*x+z*z),sqrt(x*x+z*z),z*y/sqrt(x*x+z*z))
    Eigen::Vector3f t(x * y / sqrt(x * x + z * z), sqrt(x * x + z * z), 
    z* y / sqrt(x * x + z * z));

    // Vector b = n cross product t
    auto b = normal.cross(t);

    // Matrix TBN = [t b n]
    Matrix3f TBN;
    TBN << t.x(), b.x(), x,
        t.y(), b.y(), y,
        t.z(), b.z(), z;


    // dU = kh * kn * (h(u+1/w,v)-h(u,v))
    // dV = kh * kn * (h(u,v+1/h)-h(u,v))
    float u = payload.tex_coords.x();
    float v = payload.tex_coords.y();
    float h = payload.texture->height;
    float w = payload.texture->width;
    float dU = kh * kn * (payload.texture->getColor(u + 1.0f / w, v).norm() - payload.texture->getColor(u, v).norm());
    float dV = kh * kn * (payload.texture->getColor(u, v + 1.0f / h).norm() - payload.texture->getColor(u, v).norm());

    // Vector ln = (-dU, -dV, 1)
    Eigen::Vector3f ln(-dU, -dV, 1.0f);


    point = point + (kn * normal * (payload.texture->getColor(u, v).norm()));
    // Normal n = normalize(TBN * ln)
    normal = (TBN * ln).normalized();


    Eigen::Vector3f result_color = { 0, 0, 0 };

    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *result_color* object.
        Eigen::Vector3f l = (light.position - point).normalized();      // light direction
        Eigen::Vector3f v = (eye_pos - point).normalized();     // view direction
        Eigen::Vector3f h = (v + l) / (v + l).norm();           // bisector 半程向量

        float r_2 = (light.position - point).dot(light.position - point);     // or r_2 = (l-point).squaredNorm()

        auto diffuse = kd.cwiseProduct(light.intensity / r_2 * MAX(0, normal.dot(l)));
        auto specular = ks.cwiseProduct(light.intensity / r_2 * MAX(0, std::pow(normal.dot(h), p)));
        auto ambient = ka.cwiseProduct(amb_light_intensity);

        result_color = result_color + ambient + diffuse + specular;

    }

    return result_color * 255.f;
}





int main(int argc, const char** argv)
{
    std::vector<Triangle*> TriangleList;

    float angle = 140;        // original 140
    bool command_line = false;

    std::string filename = "output.png";
    objl::Loader Loader;
    std::string obj_path = "./models/spot/";
    std::string obj_rock_path = "./models/rock/";

    // Load .obj File
     bool loadout = Loader.LoadFile("./models/spot/spot_triangulated_good.obj");      // cow model
    //bool loadout = Loader.LoadFile(obj_rock_path + "rock.obj");   // no scaling in model matrix

    for(auto mesh:Loader.LoadedMeshes)
    {
        for(int i=0;i<mesh.Vertices.size();i+=3)
        {
            Triangle* t = new Triangle();
            for(int j=0;j<3;j++)
            {
                t->setVertex(j,Vector4f(mesh.Vertices[i+j].Position.X,mesh.Vertices[i+j].Position.Y,mesh.Vertices[i+j].Position.Z,1.0));
                t->setNormal(j,Vector3f(mesh.Vertices[i+j].Normal.X,mesh.Vertices[i+j].Normal.Y,mesh.Vertices[i+j].Normal.Z));
                t->setTexCoord(j,Vector2f(mesh.Vertices[i+j].TextureCoordinate.X, mesh.Vertices[i+j].TextureCoordinate.Y));
            }
            TriangleList.push_back(t);
        }
    }

    rst::rasterizer r(700, 700);

    auto texture_path = "spot_texture.png";                     // change texture directory  
    std::string rock_texture_path = "rock.png";
    r.set_texture(Texture(obj_path + texture_path));
   // r.set_texture(Texture(obj_rock_path + rock_texture_path));

    // wrapping 
    std::function<Eigen::Vector3f(fragment_shader_payload)> active_shader = phong_fragment_shader;

    if (argc >= 2)
    {
        command_line = true;
        filename = std::string(argv[1]);

        if (argc == 3 && std::string(argv[2]) == "texture")
        {
            std::cout << "Rasterizing using the texture shader\n";
            active_shader = texture_fragment_shader;
            texture_path = "spot_texture.png";
            r.set_texture(Texture(obj_path + texture_path));
        }
        else if (argc == 3 && std::string(argv[2]) == "normal")
        {
            std::cout << "Rasterizing using the normal shader\n";
            active_shader = normal_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "phong")
        {
            std::cout << "Rasterizing using the phong shader\n";
            active_shader = phong_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "bump")
        {
            std::cout << "Rasterizing using the bump shader\n";
            active_shader = bump_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "displacement")
        {
            std::cout << "Rasterizing using the bump shader\n";
            active_shader = displacement_fragment_shader;
        }
    }

    Eigen::Vector3f eye_pos = {0,0,10};
    r.set_vertex_shader(vertex_shader);
    r.set_fragment_shader(active_shader);

    int key = 0;
    int frame_count = 0;

    if (command_line)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);
        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));

        r.draw(TriangleList);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imwrite(filename, image);

        return 0;
    }

    while(key != 27)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));     // original
        //Eigen::Vector3f axis;
        //axis << -1, 1, 0;
        //r.set_model(get_rotation_matrix(angle, axis));       // still doesn't work ??? why? idk
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, -0.1, -50));

        //r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);
        r.draw(TriangleList);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imshow("image", image);
        cv::imwrite(filename, image);
        key = cv::waitKey(10);

        if (key == 'a' )
        {
            angle -= 10;
        }
        else if (key == 'd')
        {
            angle += 10;
        }

    }
    return 0;
}
