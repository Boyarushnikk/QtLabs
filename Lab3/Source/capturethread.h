#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H


#include <QString>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>

#include "opencv2/opencv.hpp"
#include "utilities.h"

class CaptureThread: public QThread
{
    Q_OBJECT
public:
    CaptureThread(int camera, QMutex *lock);
    CaptureThread(QString videoPath, QMutex *lock);
    ~CaptureThread();
    void setRunning(bool run) {running = run; };
    void calculateFPS(cv::VideoCapture &cap);
    void startCalcFPS();
    enum VideoSavingStatus {STARTING,STARTED,STOPPING,STOPPED};
    void setVideoSavingStatus(VideoSavingStatus status) {video_saving_status = status; };
    void setMotionDetectingStatus(bool status) {motion_detecting_status = status;motion_detected = false;if(video_saving_status != STOPPED) video_saving_status = STOPPING;};
protected:
    void run() override;
signals:
    void frameCaptured(cv::Mat *data);
    void fpsChanged(float fps);
    void videoSaved(QString name);
    void statusChange(int fps, cv::Scalar mean,cv::Scalar dev);
private:
    void startSavingVideo(cv::Mat &firstFrame);
    void stopSavingVideo();
    void motionDetect(cv::Mat &frame);
    void meanDevCalc(cv::Mat frame, cv::Scalar &mean,cv::Scalar &dev);
private:
    int frame_width, frame_height;
    int cameraID;
    cv::Mat frame;
    QString videoPath;
    QString saved_video_name;
    QMutex *data_lock;
    float fps;
    bool running;
    bool fps_calculating;
    bool motion_detecting_status;
    bool motion_detected;
    VideoSavingStatus video_saving_status;
    cv::VideoWriter *video_writer;
    cv::Ptr<cv::BackgroundSubtractorMOG2> segmentor;
};

#endif // CAPTURETHREAD_H
