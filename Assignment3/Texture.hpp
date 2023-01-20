//
// Created by LEI XU on 4/27/19.
//

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H
#include "global.hpp"
#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>
class Texture{
private:
    cv::Mat image_data;

public:
    Texture(const std::string& name)
    {
        image_data = cv::imread(name);
        cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
        width = image_data.cols;
        height = image_data.rows;
    }

    int width, height;

    Eigen::Vector3f getColor(float u, float v)
    {
        //   range check 
  
        int u_img = static_cast<int>(u * width);
        int v_img = static_cast<int>((1 - v) * height);     // opencv start at left upper corner, but we use left lower corner
        if (u_img < 0) u_img = 0;
        if (u_img >= width) u_img = width - 1;
        if (v_img < 0) v_img = 0;
        if (v_img >= height) v_img = height - 1;


        //  读取的时候不是(x,y)而是(y,x)  opencv
        cv::Vec3b color = image_data.at<cv::Vec3b>(v_img, u_img);
        return Eigen::Vector3f(color[0], color[1], color[2]);


       /* auto u_img = u * width;
        auto v_img = (1 - v) * height;
        auto color = image_data.at<cv::Vec3b>(v_img, u_img);
        return Eigen::Vector3f(color[0], color[1], color[2]);*/
    }

    // enhancement
    Eigen::Vector3f getColorBilinear(float u, float v);

    int rangeSafe(int x, bool isU) {

        if (x < 0)
            return 0;

        if (isU && x >= width) {
            return width - 1;
        }
        else if (!isU && x >= height) {
            return height - 1;
        }
        return x;
    }
};
#endif //RASTERIZER_TEXTURE_H
