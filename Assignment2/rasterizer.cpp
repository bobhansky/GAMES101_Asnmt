// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>
#include <stdexcept>
#include <tuple>

#define SSAA false        // SSAA swich

rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f>& positions)
{
	auto id = get_next_id();
	pos_buf.emplace(id, positions);

	return { id };
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i>& indices)
{
	auto id = get_next_id();
	ind_buf.emplace(id, indices);

	return { id };
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f>& cols)
{
	auto id = get_next_id();
	col_buf.emplace(id, cols);

	return { id };
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
	return Vector4f(v3.x(), v3.y(), v3.z(), w);
}

// ############################MY PART****************************
static bool insideTriangle(float x, float y, const Vector3f* _v)
{
	// TODO : Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
	// 设A坐标(m,n),B坐标(p,q),则向量AB的坐标为(p-m,q-n)

	// cross product and convert 2 points to a vector

	Eigen::Vector3f p0p1(_v[1].x() - _v[0].x(), _v[1].y() - _v[0].y(), 1.0f);
	Eigen::Vector3f p1p2(_v[2].x() - _v[1].x(), _v[2].y() - _v[1].y(), 1.0f);
	Eigen::Vector3f p2p0(_v[0].x() - _v[2].x(), _v[0].y() - _v[2].y(), 1.0f);

	Eigen::Vector3f p0p(x - _v[0].x(), y - _v[0].y(), 1.0f);
	Eigen::Vector3f p1p(x - _v[1].x(), y - _v[1].y(), 1.0f);
	Eigen::Vector3f p2p(x - _v[2].x(), y - _v[2].y(), 1.0f);


	auto res1 = p0p1.cross(p0p);
	auto res2 = p1p2.cross(p1p);
	auto res3 = p2p0.cross(p2p);

	if (res1.z() > 0) {
		return res2.z() > 0.f && res3.z() > 0.f;
	}
	return res2.z() < 0.f && res3.z() < 0.f;

}
// ############################MY PART END****************************



static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
	float c1 = (x * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * y + v[1].x() * v[2].y() - v[2].x() * v[1].y()) / (v[0].x() * (v[1].y() - v[2].y()) + (v[2].x() - v[1].x()) * v[0].y() + v[1].x() * v[2].y() - v[2].x() * v[1].y());
	float c2 = (x * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * y + v[2].x() * v[0].y() - v[0].x() * v[2].y()) / (v[1].x() * (v[2].y() - v[0].y()) + (v[0].x() - v[2].x()) * v[1].y() + v[2].x() * v[0].y() - v[0].x() * v[2].y());
	float c3 = (x * (v[0].y() - v[1].y()) + (v[1].x() - v[0].x()) * y + v[0].x() * v[1].y() - v[1].x() * v[0].y()) / (v[2].x() * (v[0].y() - v[1].y()) + (v[1].x() - v[0].x()) * v[2].y() + v[0].x() * v[1].y() - v[1].x() * v[0].y());
	return { c1,c2,c3 };
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
	auto& buf = pos_buf[pos_buffer.pos_id];
	auto& ind = ind_buf[ind_buffer.ind_id];
	auto& col = col_buf[col_buffer.col_id];

	float f1 = (50 - 0.1) / 2.0;
	float f2 = (50 + 0.1) / 2.0;

	Eigen::Matrix4f mvp = projection * view * model;
	for (auto& i : ind)
	{
		Triangle t;
		Eigen::Vector4f v[] = {
				mvp * to_vec4(buf[i[0]], 1.0f),
				mvp * to_vec4(buf[i[1]], 1.0f),
				mvp * to_vec4(buf[i[2]], 1.0f)
		};

		//Homogeneous division
		for (auto& vec : v) {
			vec /= vec.w();
		}
		std::cout << "v[x] is " << v->x() << std::endl;
		std::cout << "v[y] is " << v->y() << std::endl;
		std::cout << "v[z] is " << v->z() << std::endl;
		std::cout << "v[w] is " << v->w() << std::endl;


		//Viewport transformation
		for (auto& vert : v)
		{
			vert.x() = 0.5 * width * (vert.x() + 1.0);
			vert.y() = 0.5 * height * (vert.y() + 1.0);
			vert.z() = vert.z() * f1 + f2;
		}
		std::cout << " after veiw port trans: \n";
		std::cout << "v[x] is " << v->x() << std::endl;
		std::cout << "v[y] is " << v->y() << std::endl;
		std::cout << "v[z] is " << v->z() << std::endl;
		std::cout << "v[w] is " << v->w() << std::endl;
		for (int i = 0; i < 3; ++i)
		{
			t.setVertex(i, v[i].head<3>());
			t.setVertex(i, v[i].head<3>());
			t.setVertex(i, v[i].head<3>());

		}

		auto col_x = col[i[0]];
		auto col_y = col[i[1]];
		auto col_z = col[i[2]];

		t.setColor(0, col_x[0], col_x[1], col_x[2]);
		t.setColor(1, col_y[0], col_y[1], col_y[2]);
		t.setColor(2, col_z[0], col_z[1], col_z[2]);

		
		rasterize_triangle(t);		
	}
}


// *************************************MY PART******************************
//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
	auto v = t.toVector4();

	// TODO : Find out the bounding box of current triangle.
	// iterate through the pixel and find if the current pixel is inside the triangle

	int minX = std::min(t.v[0].x(), std::min(t.v[1].x(), t.v[2].x()));
	int maxX = std::max(t.v[0].x(), std::max(t.v[1].x(), t.v[2].x()));
	int minY = std::min(t.v[0].y(), std::min(t.v[1].y(), t.v[2].y()));
	int maxY = std::max(t.v[0].y(), std::max(t.v[1].y(), t.v[2].y()));

	for (int x = minX; x <= maxX; x++) {
		for (int y = minY; y <= maxY; y++) {
			// **************SSAA**************
			if (SSAA) {
				int index = 0;
				for (float i = 0.25f; i < 1.0f; i += 0.5f) {
					for (float j = 0.25f; j < 1.0f; j += 0.5f) {
						if (insideTriangle(x + i, y + j, t.v)) {			// t.v : triangle vertices 
							auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
							float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
							float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
							z_interpolated *= w_reciprocal;

							if (z_interpolated <= depth_buf_2xSSAA[get_index(x, y)][index]) {
								depth_buf_2xSSAA[get_index(x, y)][index] = z_interpolated;
								frame_buf_2xSSAA[get_index(x, y)][index] = t.getColor();
							}
						}
						index++;
					}
				}
				// SSAA main logic part
				Eigen::Vector3f color(0, 0, 0);
				for (int index = 0; index < 4; index++) {
					color += frame_buf_2xSSAA[get_index(x, y)][index];
				}
				// averaging
				color /= 4;
				set_pixel(Eigen::Vector3f(x, y, 1.0f), color);
			}
			// **************SSAA ENDS**************

			// none SSAA
			else {
				if (insideTriangle(x + 0.5f, y + 0.5f, t.v)) {
					// tuple type   I change to C++ 20 
					auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
					float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
					float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
					z_interpolated *= w_reciprocal;

					// TODO : set the current pixel (use the set_pixel function) to the color of the triangle 
					// (use getColor function) if it should be painted.
					if (z_interpolated <= depth_buf[get_index(x, y)]) {
						depth_buf[get_index(x, y)] = z_interpolated;
						set_pixel(Eigen::Vector3f(x, y, 1), t.getColor());
					}
				}
			}

		}
	}

	// If insideTriangle, use the following code to get the interpolated z value.
	//auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
	//float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
	//float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
	//z_interpolated *= w_reciprocal;

	// TODO : set the current pixel (use the set_pixel function) to the color of the triangle (use getColor function) if it should be painted.
}


// *************************************MY PART END******************************



void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
	model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
	view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
	projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
	if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
	{
		if (SSAA) {
			for (int i = 0; i < frame_buf_2xSSAA.size(); i++) {
				frame_buf_2xSSAA[i].resize(4);
				// set depth info to infinity   std::numeric_limits<float>::infinity()
				std::fill(frame_buf_2xSSAA[i].begin(), frame_buf_2xSSAA[i].end(), Eigen::Vector3f(0,0,0));
			}
		}
		else {
			std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f( 0, 0, 0 ));
		}
	}
	if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
	{
		if (SSAA) {
			for (int i = 0; i < depth_buf_2xSSAA.size(); i++) {
				depth_buf_2xSSAA[i].resize(4);
				// set depth info to -infinity   std::numeric_limits<float>::infinity()
				std::fill(depth_buf_2xSSAA[i].begin(), depth_buf_2xSSAA[i].end(), std::numeric_limits<float>::infinity());
			}
		}
		else {
			std::fill(depth_buf.begin(), depth_buf.end(),std::numeric_limits<float>::infinity());
		}
	}
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
	frame_buf.resize(w * h);   // resize(int n,element)表示调整容器vv的大小为n，扩容后的每个元素的值为element，默认为0
	depth_buf.resize(w * h);

	// enhancement SSAA
	if (SSAA) {
		depth_buf_2xSSAA.resize(w * h, std::vector<float>(4,std::numeric_limits<float>::infinity()));
		frame_buf_2xSSAA.resize(w * h, std::vector<Eigen::Vector3f>(4,Eigen::Vector3f(0,0,0)));
	}
	// enhancement SSAA ends
}

int rst::rasterizer::get_index(int x, int y)
{
	return (height - 1 - y) * width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
	//old index: auto ind = point.y() + point.x() * width;
	auto ind = (height - 1 - point.y()) * width + point.x();		// original
	frame_buf[ind] = color;			// original

}

// clang-format on


// enhancement:
// enlarge the buffer: make 4 more buffers for each pixel, ex: pixel at (1,1)
// then 1.75,1.25      1.75,1.75
//              (1.5,1.5)           not including the center point, this is just used for clarification & understanding
//      1.25,1.25      1.75,1.25



// referrence
// https://zhuanlan.zhihu.com/p/425153734