#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <vector>
std::vector<cv::Point2f> control_points;

void mouse_handler(int event, int x, int y, int flags, void *userdata)
{
    if (event == cv::EVENT_LBUTTONDOWN && control_points.size() < 4)
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", "
                  << y << ")" << '\n';
        control_points.emplace_back(x, y);
    }
}

void naive_bezier(const std::vector<cv::Point2f> &points, cv::Mat &window)
{
    auto &p_0 = points[0];
    auto &p_1 = points[1];
    auto &p_2 = points[2];
    auto &p_3 = points[3];

    for (double t = 0.0; t <= 1.0; t += 0.001)
    {
        auto point = std::pow(1 - t, 3) * p_0 + 3 * t * std::pow(1 - t, 2) * p_1 +
                     3 * std::pow(t, 2) * (1 - t) * p_2 + std::pow(t, 3) * p_3;

        window.at<cv::Vec3b>(point.y, point.x)[2] = 255;
    }
}

int combination_count(int m, int n)
{
    if (m < 0 || m > n)
    {
        return 0;
    }

    m = std::min(m, n - m);

    long long result = 1;

    for (int i = 1; i <= m; ++i)
    {
        result = result * (n - m + i) / i;
    }

    return static_cast<int>(result);
}

float Bernstein_polynomial(int i, int n, float t)
{
    return combination_count(i, n) * std::pow(t, i) * std::pow(1 - t, n - i);
}

cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t)
{
    // TODO: Implement de Casteljau's algorithm
    if (control_points.size() == 1)
    {
        return control_points[0];
    }
    cv::Point2f new_control_point;
    int n = control_points.size();
    std::cout << "n = " << n << std::endl;
    for (size_t i = 0; i < n; ++i)
    {
        new_control_point += Bernstein_polynomial(i, n - 1, t) * control_points[i];
    }
    return new_control_point;
}
cv::Point2f recursive_bezier2(const std::vector<cv::Point2f> &control_points, float t)
{
    // 拷贝一份，原地覆盖计算
    std::vector<cv::Point2f> points = control_points;
    int n = points.size();

    for (int r = 1; r < n; ++r) // 第 r 层
    {
        for (int i = 0; i < n - r; ++i) // 每层少一个点
        {
            points[i] = (1 - t) * points[i] + t * points[i + 1];
        }
    }

    return points[0];
}
void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window)
{
    // TODO: Iterate through all t = 0 to t = 1 with small steps, and call de Casteljau's
    // recursive Bezier algorithm.
    for (int i = 0.0; i <= 1000.0; i++)
    {
        float t = i / 1000.0;
        auto point = recursive_bezier2(control_points, t);
        window.at<cv::Vec3b>(point.y, point.x)[2] = 255;
    }
}

int main()
{
    cv::Mat window = cv::Mat(700, 700, CV_8UC3, cv::Scalar(0));
    cv::cvtColor(window, window, cv::COLOR_BGR2RGB);
    cv::namedWindow("Bezier Curve", cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);

    std::cout << combination_count(3, 4) << std::endl;
    int key = -1;
    while (key != 27)
    {
        for (auto &point : control_points)
        {
            cv::circle(window, point, 3, {255, 255, 255}, 3);
        }

        // 当前程序要求用户输入 4 个控制点；当控制点数量达到 4 个时，
        // 就可以绘制一条三次 Bezier 曲线。
        if (control_points.size() == 4)
        {
            // 使用基于 Bernstein 多项式的直接计算方法绘制 Bezier 曲线。
            // 这里传入控制点和用于显示的图像窗口。
            // naive_bezier(control_points, window);

            // 另一种绘制方式是调用 de Casteljau 算法实现的 bezier 函数。
            // 当前暂时注释掉，取消注释后即可使用该算法绘制曲线。
            bezier(control_points, window);

            // 将绘制完成的曲线显示在窗口中。
            cv::imshow("Bezier Curve", window);

            // 将当前窗口图像保存为 PNG 文件，便于后续查看或提交作业。
            cv::imwrite("my_bezier_curve.png", window);

            // 等待用户按下任意键。
            // 参数为 0 表示无限等待，不会自动继续执行。
            key = cv::waitKey(0);

            // 曲线绘制和保存完成后退出程序，不再继续主循环。
            return 0;
        }

        // 控制点数量还不足 4 个时，实时刷新窗口，显示已经输入的控制点。
        cv::imshow("Bezier Curve", window);

        // 等待 20 毫秒，同时处理 OpenCV 窗口事件（如鼠标点击和窗口刷新）。
        // 返回值是用户按下的键的键码；如果没有按键，通常返回 -1。
        key = cv::waitKey(20);
    }

    return 0;
}
