// clang-format off
#include <iostream>
#include <opencv2/opencv.hpp>
#include "rasterizer.hpp"
#include "global.hpp"
#include "Triangle.hpp"

constexpr double MY_PI = 3.1415926;

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

// default model_matrix
Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    return model;
}

Eigen::Matrix4f get_translation_matrix(float disx, float disy, float disz) {
    Eigen::Matrix4f w;
    w << 1, 0, 0, disx,
        0, 1, 0, disy,
        0, 0, 1, disz,
        0, 0, 0, 1;
    return w;
}

Eigen::Matrix4f get_rotateZ_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the model matrix for rotating the triangle around the Z axis.
    // Then return it.
    // rotation_angle is in degree, cos sin takes radians

    Eigen::Matrix4f something;
    float ang = (rotation_angle / 180) * MY_PI;
    something << std::cos(ang), -std::sin(ang), 0, 0,
        std::sin(ang), std::cos(ang), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1;

    model = something * model;

    return model;
}


// my own enhancement           it doesnt work
// rotate around arbitrary axis n passing through the origin by angle 
Eigen::Matrix4f get_rotation_matrix(float rota_angle, Eigen::Vector3f& axis) {
    if (axis.x() - 0.0 < 0.0001 && axis.y() - 0.0 < 0.0001 && axis.z() - 0.0 < 0.0001) {
        return Eigen::Matrix4f::Identity();
    }
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

    float t = std::tan((eye_fov / 2) * MY_PI / 180) * ( -zNear);    // tan() takes radians
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

    projection = ortho_proj * per_to_ortho;      


    return projection;
 
}

int main(int argc, const char** argv)
{
    float angle = 0;
    float displacement_y = 0, displacement_x = 0 ,displacement_z = 0;
    bool command_line = false;       // initial false
    std::string filename = "output.png";

    if (argc == 2)
    {
        command_line = true;
        filename = std::string(argv[1]);
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0,0,5};


    std::vector<Eigen::Vector3f> pos
            {
                    {2, 0, -2},
                    {0, 2, -2},
                    {-2, 0, -2},
                    {3.5, -1, -5},
                    {2.5, 1.5, -5},
                    {-1, 0.5, -5}
            };

    std::vector<Eigen::Vector3i> ind
            {
                    {0, 1, 2},
                    {3, 4, 5}
            };

    // colors
    std::vector<Eigen::Vector3f> cols
            {
                    {217.0, 238.0, 185.0},
                    {217.0, 238.0, 185.0},
                    {217.0, 238.0, 185.0},
                    {185.0, 217.0, 238.0},      // z==-5 triangle  blue
                    {185.0, 217.0, 238.0},
                    {185.0, 217.0, 238.0}
                    
            };

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);
    auto col_id = r.load_colors(cols);

    int key = 0;
    int frame_count = 0;

   /* if (command_line)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0f, 1.0f, 0.1f, 50.0f));

        r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imwrite(filename, image);

        return 0;
    }*/

    while(key != 27)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);
        
        r.set_model(get_model_matrix(angle));             // default
       
        // r.set_model(get_translation_matrix(displacement_x, displacement_y, displacement_z));

        Eigen::Vector3f axis;
        axis << 0, 0, 0;
        // r.set_model(get_rotation_matrix(angle, axis));      // unexpected result  ???    // get_rotation_matrix is wrong ?
        // r.set_model(get_rotateZ_matrix(angle));     // but this works

        r.set_view(get_view_matrix(eye_pos));
        // znear zFar: Number Distance to the near clipping plane along the -Z axis.
        r.set_projection(get_projection_matrix(45, 1.f, 0.1f, 50.f)); 

        r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << '\n';

        // rotation
        if (key == 'q') {
            angle += 20;
        }
        else if (key == 'e') {
            angle -= 20;
        }

        // displacement
        else if (key == 'w') {
            displacement_y += 0.5;
        }
        else if (key == 's') {
            displacement_y -= 0.5;
        }
        else if (key == 'a') {
            displacement_x -= 0.5;
        }
        else if (key == 'd') {
            displacement_x += 0.5;
        }
        else if (key == 'z') {
            displacement_z += 1;
        }
        else if (key == 'c') {
            displacement_z -= 1;
        }
    }

    return 0;
}
// clang-format on




// https://games-cn.org/forums/topic/zuoye1-gaojierenwu-raoxyzhouxuanzhuanxiaoguotu-shifouzhengque/  check it  9/24/2022