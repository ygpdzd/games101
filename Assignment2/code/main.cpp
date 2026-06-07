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

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    float radian = rotation_angle / 180.0f * MY_PI;
    Eigen::Matrix4f rotate;
    rotate << std::cos(radian), -std::sin(radian), 0, 0,
        std::sin(radian), std::cos(radian), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1;

    model = rotate * model;

    // 添加轻微缩放，让三角形更大，边缘更明显
    Eigen::Matrix4f scale;
    scale << 1.5, 0, 0, 0,
             0, 1.5, 0, 0,
             0, 0, 1, 0,
             0, 0, 0, 1;

    model = scale * model;
    return model;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio, float zNear, float zFar)
{
    // TODO: Copy-paste your implementation from the previous assignment.
    Eigen::Matrix4f projection;
    auto a = eye_fov/180.0f*MY_PI;
    auto n = zNear;
    auto f = zFar;
    auto t = -n*std::tan(a/2.0f);
    auto r = t*aspect_ratio;
    auto l = -r;
    auto b = -t;

    projection << n, 0, 0, 0,
                  0, n, 0, 0,
                  0, 0, n + f, n * f,
                  0, 0, 1, 0;

    Eigen::Matrix4f orthographic_translate;
    orthographic_translate << 1, 0, 0, -(r+l)/2,
                   0, 1, 0, -(t+b)/2,
                   0, 0, 1, -(n+f)/2,
                   0, 0, 0, 1;

    Eigen::Matrix4f orthographic_scale ;
    orthographic_scale <<
                2/(r-l), 0, 0, 0,
                0, 2/(t-b), 0, 0,
                0, 0, 2/(n-f), 0,
                0, 0, 0, 1;


    return orthographic_scale * orthographic_translate * projection;
}

int main(int argc, const char** argv)
{
    float angle = 0;
    bool command_line = false;
    std::string filename = "output.png";

    if (argc == 2)
    {
        command_line = true;
        filename = std::string(argv[1]);
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0,0,5};


    // 创建更适合展示锯齿的几何体：细长的三角形和斜边
    std::vector<Eigen::Vector3f> pos
            {
                    // 第一个三角形：锐角三角形，边缘更陡峭
                    {2, 0, -2},
                    {0, 2, -2},
                    {-2, 0, -2},

                    // 第二个三角形：更小的三角形，放在远处
                    {3.5, -1, -5},
                    {2.5, 1.5, -5},
                    {-1, 0.5, -5},

                    // 新增：一个非常细长的三角形（更容易看到锯齿）
                    {1.5, 0, -1.5},
                    {0, 1.5, -1.5},
                    {-1.5, 0, -1.5}
            };

    std::vector<Eigen::Vector3i> ind
            {
                    {0, 1, 2},
                    {3, 4, 5},
                    {6, 7, 8}  // 新增的三角形
            };

    std::vector<Eigen::Vector3f> cols
            {
                    {217.0, 238.0, 185.0},
                    {217.0, 238.0, 185.0},
                    {217.0, 238.0, 185.0},
                    {185.0, 217.0, 238.0},
                    {185.0, 217.0, 238.0},
                    {185.0, 217.0, 238.0},
                    {255.0, 100.0, 100.0},  // 红色，更醒目
                    {255.0, 100.0, 100.0},
                    {255.0, 100.0, 100.0}
            };

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);
    auto col_id = r.load_colors(cols);

    int key = 0;
    int frame_count = 0;
    bool show_zoomed = false;  // 是否显示放大视图

    if (command_line)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imwrite(filename, image);

        return 0;
    }

    while(key != 27)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        // 在图像上显示 SSAA 状态和操作提示
        std::string status_text = r.is_ssaa_enabled() ? "SSAA: ON" : "SSAA: OFF";
        cv::putText(image, status_text, cv::Point(10, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);

        cv::putText(image, "Press 'S': Toggle SSAA", cv::Point(10, 60),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
        cv::putText(image, "Press 'Z': Toggle Zoom View", cv::Point(10, 85),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
        cv::putText(image, "Press 'A/D': Rotate", cv::Point(10, 110),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);

        // 如果启用放大视图，显示局部放大区域
        if (show_zoomed) {
            // 选择一个三角形的边缘区域进行放大（例如第一个三角形的右上边缘）
            int zoom_x = 350, zoom_y = 200;  // 放大区域的左上角
            int zoom_size = 100;  // 放大区域的大小

            cv::Rect zoom_rect(zoom_x, zoom_y, zoom_size, zoom_size);
            cv::Mat zoomed_region = image(zoom_rect).clone();

            // 放大 4 倍
            cv::Mat enlarged;
            cv::resize(zoomed_region, enlarged, cv::Size(zoom_size * 4, zoom_size * 4),
                      0, 0, cv::INTER_NEAREST);

            // 在右下角显示放大视图
            int display_x = image.cols - zoom_size * 4 - 20;
            int display_y = image.rows - zoom_size * 4 - 20;

            cv::Rect display_rect(display_x, display_y, zoom_size * 4, zoom_size * 4);
            enlarged.copyTo(image(display_rect));

            // 绘制边框
            cv::rectangle(image, display_rect, cv::Scalar(255, 255, 0), 2);
            cv::putText(image, "4x Zoom", cv::Point(display_x + 10, display_y + 25),
                       cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 0), 2);

            // 在原图上标记放大区域
            cv::rectangle(image, zoom_rect, cv::Scalar(255, 255, 0), 2);
        }

        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << '\n';
        if (key == 'a')
        {
            angle += 10;
        }
        else if (key == 'd')
        {
            angle -= 10;
        }
        else if (key == 's' || key == 'S')
        {
            r.toggle_ssaa();
            std::cout << "SSAA toggled: " << (r.is_ssaa_enabled() ? "ON" : "OFF") << std::endl;
        }
        else if (key == 'z' || key == 'Z')
        {
            show_zoomed = !show_zoomed;
            std::cout << "Zoom view: " << (show_zoomed ? "ON" : "OFF") << std::endl;
        }
    }

    return 0;
}
// clang-format on