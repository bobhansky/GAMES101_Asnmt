//
// Created by LEI XU on 4/27/19.
//
#include"Texture.hpp"
#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>

Eigen::Vector3f Texture::getColorBilinear(float u, float v) {
	// 纹理坐标的范围是 0 到 1 之间
    float u_img = u * width;
    float v_img = (1 - v) * height;     // cv原点在左上角，而我们定义的光栅化原点在左下角。这导致我们的v变成了1-v

    int u_min = rangeSafe(floor(u_img), true);
    int u_max = rangeSafe(ceil(u_img), true);
    int v_min = rangeSafe(floor(v_img), false);
    int v_max = rangeSafe(ceil(v_img), false);

    cv::Vec3b U00 = image_data.at<cv::Vec3b>(v_max, u_min);
    cv::Vec3b U10 = image_data.at<cv::Vec3b>(v_max, u_max);
    cv::Vec3b U01 = image_data.at<cv::Vec3b>(v_min, u_min);
    cv::Vec3b U11 = image_data.at<cv::Vec3b>(v_min, u_max);

    float lerp_s = (u_img - u_min) / (u_max - u_min);       // ratio 
    float lerp_t = (v_img - v_min) / (v_max - v_min);

    cv::Vec3b cTop = (1 - lerp_s) * U01 + lerp_s * U11;
    cv::Vec3b cBot = (1 - lerp_s) * U00 + lerp_s * U10;

    cv::Vec3b P = (1 - lerp_t) * cTop + lerp_t * cBot;
    return Eigen::Vector3f(P[0], P[1], P[2]);
}


// incompleted