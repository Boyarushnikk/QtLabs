#include "erode_plugin.h"

QString erode_plugin::name()
{
    return  "Erode";
}

void erode_plugin::edit(const cv::Mat &input, cv::Mat &output)
{
     cv::erode(input, output, cv::Mat());
}
