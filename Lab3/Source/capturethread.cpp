#include "capturethread.h"

CaptureThread::CaptureThread(int camera, QMutex *lock):
    running(false), cameraID(camera), videoPath(""), data_lock(lock)
{
    fps_calculating = false;
    fps = 0.0;

    frame_width = frame_height = 0;
    video_saving_status = STOPPED;
    saved_video_name = "";
    video_writer = nullptr;
    motion_detecting_status = false;

}

CaptureThread::CaptureThread(QString videoPath, QMutex *lock):
    running(false), cameraID(-1), videoPath(videoPath), data_lock(lock)
{
    fps_calculating = false;
    fps = 0.0;

    frame_width = frame_height = 0;
    video_saving_status = STOPPED;
    saved_video_name = "";
    video_writer = nullptr;
    motion_detecting_status = false;

}

CaptureThread::~CaptureThread()
{

}

void CaptureThread::calculateFPS(cv::VideoCapture &cap)
{
    const int count_to_read = 100;
    cv::Mat tmp_frame;
    QElapsedTimer  timer;
    timer.start();
    for(int i = 0; i < count_to_read; i++) {
        cap >> tmp_frame;
    }
    int elapsed_ms = timer.elapsed();
    fps = count_to_read / (elapsed_ms / 1000.0);
    fps_calculating = false;
    emit fpsChanged(fps);
}

void CaptureThread::startCalcFPS()
{
    fps_calculating = true;
}

void CaptureThread::run()
{
    running = true;
    cv::VideoCapture cap(cameraID);
    cv::Mat tmp_frame;
    frame_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    frame_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    segmentor = cv::createBackgroundSubtractorMOG2(500, 16, true);

    int frames = 0;
    QTimer *timer = new QTimer();

    while(running) {
        cap >> tmp_frame;
        if (tmp_frame.empty()) {
            break;
        }
        if(motion_detecting_status) {
            motionDetect(tmp_frame);
         }

        if(fps_calculating) {
             calculateFPS(cap);
         }
        if(video_saving_status == STARTING)
            startSavingVideo(tmp_frame);
        if(video_saving_status == STARTED)
            video_writer->write(tmp_frame);
        if(video_saving_status == STOPPING)
             stopSavingVideo();

        cvtColor(tmp_frame, tmp_frame, cv::COLOR_BGR2RGB);
        data_lock->lock();
        frame = tmp_frame;
        data_lock->unlock();
        emit frameCaptured(&frame);
        if(!timer->remainingTime() || (timer->remainingTime() == -1)) {
            fps = frames / (1000 / 1000.0);
            frames = 0;
            timer->start(1000);
        }else frames++;
        cv::Scalar mean={0.};
        cv::Scalar dev={0.};
        meanDevCalc(tmp_frame,mean,dev);
        emit statusChange(fps, mean, dev);
    }
    cap.release();
    running = false;
}

void CaptureThread::startSavingVideo(cv::Mat &firstFrame)
{
    saved_video_name = Utilities::newSavedVideoName();
    QString cover = Utilities::getSavedVideoPath(saved_video_name, "jpg");
    cv::imwrite(cover.toStdString(), firstFrame);
    video_writer = new cv::VideoWriter(
            Utilities::getSavedVideoPath(saved_video_name, "avi").toStdString(),
            cv::VideoWriter::fourcc('M','J','P','G'),
            fps? fps: 30,
            cv::Size(frame_width,frame_height),
            true);
    video_saving_status = STARTED;
}

void CaptureThread::stopSavingVideo()
{
    video_saving_status = STOPPED;
    video_writer->release();
    delete video_writer;
    video_writer = nullptr;
    emit videoSaved(saved_video_name);
}

void CaptureThread::motionDetect(cv::Mat &frame)
{
    cv::Mat fgmask;
    segmentor->apply(frame, fgmask);
    if (fgmask.empty()) {
        return;
    }
    cv::threshold(fgmask, fgmask, 25, 255, cv::THRESH_BINARY);
    int noise_size = 9;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(noise_size, noise_size));
    cv::erode(fgmask, fgmask, kernel);
    kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(noise_size, noise_size));
    cv::dilate(fgmask, fgmask, kernel, cv::Point(-1,-1), 3);
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(fgmask, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    bool has_motion = contours.size() > 0;
    if(!motion_detected && has_motion) {
        motion_detected = true;
        setVideoSavingStatus(STARTING);
    } else if (motion_detected && !has_motion) {
        motion_detected = false;
        setVideoSavingStatus(STOPPING);
    }
    cv::Scalar color = cv::Scalar(0, 0, 255); // red
    for(size_t i = 0; i < contours.size(); i++) {
        cv::Rect rect = cv::boundingRect(contours[i]);
        cv::rectangle(frame, rect, color, 1);
    }
}

void CaptureThread::meanDevCalc(cv::Mat frame, cv::Scalar &mean, cv::Scalar &dev)
{
    int rows = frame.rows,cols = frame.cols;;
    cv::Vec3b* currentRow;
    for(int i = 0; i < rows; i++)
    {
        currentRow = frame.ptr<cv::Vec3b>(i);
        for (int j = 0; j < cols; j++)
        {
            mean[2] += currentRow[j][0];
            mean[1] += currentRow[j][1];
            mean[0] += currentRow[j][2];
        }
     }
    mean[2] = mean[2] / (cols*rows);
    mean[1] = mean[1] / (cols*rows);
    mean[0] = mean[0] / (cols*rows);
    for(int i = 0; i < rows; i++)
    {
        currentRow = frame.ptr<cv::Vec3b>(i);
        for (int j = 0; j < cols; j++)
        {
            dev[2] += (currentRow[j][0] - mean[2]) * (currentRow[j][0] - mean[2]);
            dev[1]  += (currentRow[j][1] - mean[1]) * (currentRow[j][1] - mean[1]);
            dev[0] += (currentRow[j][2] - mean[2]) * (currentRow[j][2] - mean[2]);
        }
    }
    dev[2] = sqrt(dev[2] / (cols*rows - 1));
    dev[1] = sqrt(dev[1] / (cols*rows - 1));
    dev[0] = sqrt(dev[0] / (cols*rows - 1));
}
