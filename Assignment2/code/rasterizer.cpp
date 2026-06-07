// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}


static bool insideTriangle(float x, float y, const Vector3f* _v)
{
    // TODO: Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
    Vector2f o = Vector2f(x, y); // 齐次坐标
    Vector2f a = _v[0].head(2) - _v[1].head(2);
    Vector2f b = _v[1].head(2) - _v[2].head(2);
    Vector2f c = _v[2].head(2) - _v[0].head(2);
    Vector2f p0 = o - _v[0].head(2);
    Vector2f p1 = o - _v[1].head(2);
    Vector2f p2 = o - _v[2].head(2);

    float cross1 = a.x() * p0.y() - a.y() * p0.x();
    float cross2 = b.x() * p1.y() - b.y() * p1.x();
    float cross3 = c.x() * p2.y() - c.y() * p2.x();

    // 点在所有线的同侧则在三角形外
    if ((cross1 > 0 && cross2 > 0 && cross3 > 0) || (cross1 < 0 && cross2 < 0 && cross3 < 0))
        return true;
    return false;


}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
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
        //Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
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

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    auto v = t.toVector4();

    // 如果未启用 SSAA，使用传统的单采样光栅化
    if (!use_ssaa) {
        int min_x = std::min(v[0].x(), std::min(v[1].x(), v[2].x()));
        int max_x = std::max(v[0].x(), std::max(v[1].x(), v[2].x()));
        int min_y = std::min(v[0].y(), std::min(v[1].y(), v[2].y()));
        int max_y = std::max(v[0].y(), std::max(v[1].y(), v[2].y()));

        min_x = std::max(0, min_x);
        min_y = std::max(0, min_y);
        max_x = std::min(width - 1, max_x);
        max_y = std::min(height - 1, max_y);

        for (int i = min_x; i <= max_x; i++) {
            for (int j = min_y; j <= max_y; j++) {
                float x = i + 0.5f;
                float y = j + 0.5f;

                if (insideTriangle(x, y, t.v)) {
                    int ind = get_index(i, j);
                    auto [alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
                    float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                    float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                    z_interpolated *= w_reciprocal;

                    if (z_interpolated < depth_buf[ind]) {
                        depth_buf[ind] = z_interpolated;
                        Vector3f color = t.getColor();
                        frame_buf[ind] = color;
                    }
                }
            }
        }
        return;
    }

    // 启用 SSAA 时的多采样光栅化
    int min_x = std::min(v[0].x(), std::min(v[1].x(), v[2].x()));
    int max_x = std::max(v[0].x(), std::max(v[1].x(), v[2].x()));
    int min_y = std::min(v[0].y(), std::min(v[1].y(), v[2].y()));
    int max_y = std::max(v[0].y(), std::max(v[1].y(), v[2].y()));

    min_x = std::max(0, min_x);
    min_y = std::max(0, min_y);
    max_x = std::min(width - 1, max_x);
    max_y = std::min(height - 1, max_y);

    for (int i = min_x; i <= max_x; i++) {
        for (int j = min_y; j <= max_y; j++) {
            for (int s = 0; s < sample_count; s++) {
                Eigen::Vector2f offset = get_sample_offset(s);
                float sample_x = i + offset.x();
                float sample_y = j + offset.y();

                if (insideTriangle(sample_x, sample_y, t.v)) {
                    int ind = get_index(i, j);
                    auto [alpha, beta, gamma] = computeBarycentric2D(sample_x, sample_y, t.v);
                    float w_reciprocal = 1.0 / (alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                    float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                    z_interpolated *= w_reciprocal;

                    if (z_interpolated < sample_depths[ind][s]) {
                        sample_depths[ind][s] = z_interpolated;
                        Vector3f color = t.getColor();
                        sample_colors[ind][s] = color;
                    }
                }
            }
        }
    }
}

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
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});

        // 如果启用了 SSAA，清空采样颜色缓冲区
        if (use_ssaa) {
            for (auto& pixel_samples : sample_colors) {
                std::fill(pixel_samples.begin(), pixel_samples.end(), Eigen::Vector3f{0, 0, 0});
            }
        }
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());

        // 如果启用了 SSAA，清空采样深度缓冲区
        if (use_ssaa) {
            for (auto& pixel_samples : sample_depths) {
                std::fill(pixel_samples.begin(), pixel_samples.end(), std::numeric_limits<float>::max());
            }
        }
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
    // 初始化 SSAA 缓冲区
    sample_colors.resize(w * h);
    sample_depths.resize(w * h);

    for (int i = 0; i < w * h; ++i) {
        for (int j = 0; j < sample_count; ++j) {
            sample_colors[i][j] = Eigen::Vector3f(0, 0, 0);
            sample_depths[i][j] = std::numeric_limits<float>::max();
        }
    }
}
Eigen::Vector2f rst::rasterizer::get_sample_offset(int sample_index) {
    // 2x2 grid offsets
    switch (sample_index) {
        case 0: return Eigen::Vector2f(0.25f, 0.25f);
        case 1: return Eigen::Vector2f(0.75f, 0.25f);
        case 2: return Eigen::Vector2f(0.25f, 0.75f);
        case 3: return Eigen::Vector2f(0.75f, 0.75f);
        default: return Eigen::Vector2f(0.5f, 0.5f);
    }
}
int rst::rasterizer::get_index(int x, int y)
{
    return (height-1-y)*width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

// clang-format on
