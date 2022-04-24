#include "median_plugin.h"

QString median_plugin::name()
{
    return "Median";
}

void median_plugin::edit(const cv::Mat &input, cv::Mat &output)
{
    cv::Mat tmp;
    cv::copyMakeBorder(input,tmp,1,1,1,1,1,cv::BORDER_REFLECT);
    std::vector<int> B,G,R;
    for(int row = 1; row < input.rows-1;row++)
    {
        cv::Vec3b* ptrdst = output.ptr<cv::Vec3b>(row);
        for(int col = 1; col <  input.cols-1; col++)
        {
            for(int r = row-1;r <= row+1;r++)
            {
                cv::Vec3b* ptrsrc = tmp.ptr<cv::Vec3b>(r);
                for(int c = col-1;c <= col+1;c++)
                {
                    B.push_back(ptrsrc[c][0]);
                    G.push_back(ptrsrc[c][1]);
                    R.push_back(ptrsrc[c][2]);
                }
            }
            std::sort(B.begin(),B.end());
            std::sort(G.begin(),G.end());
            std::sort(R.begin(),R.end());
            cv::Vec3b outPix(B[4],G[4],R[4]);
            ptrdst[col] = outPix;
            B.clear();
            G.clear();
            R.clear();
        }
    }
}



