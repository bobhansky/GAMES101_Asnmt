//
// Created by LEI XU on 4/27/19.
//
#include"Texture.hpp"
#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>




// 10/16/2022
// 
// if the we want to calculate textrue color at u, v
// we take ceiling, floor of both u_img and v_img	(texutre position)

Eigen::Vector3f Texture::getColorBilinear(float u, float v)
{	
	// get these u&v_images
	float u_img = u * width;
	float v_img = (1 - v) * height;

	int uMin = rangeSafe(floor(u_img), true);
	int uMax = rangeSafe(ceil(u_img), true);
	int vMin = rangeSafe(floor(v_img), false);
	int vMax = rangeSafe(ceil(v_img), false);

	// opencv  y,x
	cv::Vec3b U00 = image_data.at<cv::Vec3b>(vMax, uMin);
	cv::Vec3b U10 = image_data.at<cv::Vec3b>(vMax, uMax);
	cv::Vec3b U01 = image_data.at<cv::Vec3b>(vMin, uMin);
	cv::Vec3b U11 = image_data.at<cv::Vec3b>(vMin, uMax);

	// horizontal interpolation 
	float lerp_s = (u_img - uMin) / (uMax - uMin);
//	Eigen::Vector3f u0 = U00 + lerp_s * (U10 - U00);
	cv::Vec3b u0 = (1 - lerp_s) * U00 + lerp_s * U10;

//	Eigen::Vector3f u1 = U01 + lerp_s * (U11 - U01);
	cv::Vec3b u1 = (1 - lerp_s) * U01 + lerp_s * U11;

	// verticle interpolation -> res
	float lerp_t = (v_img - vMin) / (vMax - vMin);
//	res = u0 + lerp_t * (u1 - u0);
	cv::Vec3b P = (1 - lerp_t) * u1 + lerp_t * u0;
	return Eigen::Vector3f(P[0], P[1], P[2]);
}
