#include "sharpen_plugin.h"

QString sharpen_plugin::name()
{
    return "Sharpen";
}

void sharpen_plugin::edit(const cv::Mat &input, cv::Mat &output)
{
    int intensity = 2;
    cv::Mat smoothed;
    GaussianBlur(input, smoothed, cv::Size(9, 9), 0);
    output = input + (input - smoothed) * intensity;
}
