#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    initUI();
    createActions();
    populateSavedList();
    data_lock = new QMutex();
}

MainWindow::~MainWindow()
{
}

void MainWindow::initUI()
{
    this->resize(1000, 800);

     // setup menubar
    fileMenu = menuBar()->addMenu("&File");

     // setup status bar
    mainStatusBar = statusBar();
    mainStatusLabel = new QLabel(mainStatusBar);
    mainStatusBar->addPermanentWidget(mainStatusLabel);
    mainStatusLabel->setText("Gazer is Ready");

    QGridLayout *main_layout = new QGridLayout();
#ifdef GAZER_USE_QT_CAMERA
    QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const QCameraDevice &cameraDevice : cameras) {
        camera = new QCamera(cameraDevice);
        break;
    }
    // I have two cameras and use the second one here
    captureSession = new QMediaCaptureSession;
    captureSession->setCamera(camera);
    preview = new QVideoWidget;
    main_layout->addWidget(preview, 0, 0, 12, 1);
    captureSession->setVideoOutput(preview);
    //preview->show();
#else
    imageScene = new QGraphicsScene(this);
    imageView = new QGraphicsView(imageScene);
    main_layout->addWidget(imageView, 0, 0, 12, 1);
 #endif
    QGridLayout *tools_layout = new QGridLayout();
    main_layout->addLayout(tools_layout, 12, 0, 1, 1);
    monitorCheckBox = new QCheckBox(this);
    monitorCheckBox->setText("Monitor On/Off");
    connect(monitorCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateMonitorStatus(int)));
    tools_layout->addWidget(monitorCheckBox, 0, 0);
    recordButton = new QPushButton(this);
    recordButton->setText("Record");
    tools_layout->addWidget(recordButton, 0, 1, Qt::AlignHCenter);
    tools_layout->addWidget(new QLabel(this), 0, 2);

     // list of saved videos
    saved_list = new QListView(this);
    saved_list->setViewMode(QListView::IconMode);
    saved_list->setResizeMode(QListView::Adjust);
    saved_list->setSpacing(5);
    saved_list->setWrapping(false);
    list_model = new QStandardItemModel(this);
    saved_list->setModel(list_model);
    main_layout->addWidget(saved_list, 13, 0, 4, 1);

    connect(recordButton, SIGNAL(clicked(bool)), this, SLOT(recordingStartStop()));

    QWidget *widget = new QWidget();
    widget->setLayout(main_layout);
    setCentralWidget(widget);
}

void MainWindow::createActions()
{
    cameraInfoAction = new QAction("&Camera Info", this);
    connect(cameraInfoAction, SIGNAL(triggered(bool)), this, SLOT(showCameraInfo()));
    fileMenu->addAction(cameraInfoAction);

    openCameraAction = new QAction("&Open Camera", this);
    connect(openCameraAction, SIGNAL(triggered(bool)), this, SLOT(openCamera()));
    fileMenu->addAction(openCameraAction);

    calcFPSAction = new QAction("&Calculate FPS", this);
    fileMenu->addAction(calcFPSAction);
    connect(calcFPSAction, SIGNAL(triggered(bool)), this, SLOT(calculateFPS()));
}

void MainWindow::showCameraInfo()
{
    QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    QString info = QString("Available Cameras: \n");
    for (const QCameraDevice &cameraDevice : cameras) {
        info += " - " + cameraDevice.description() + ": ";
        info += cameraDevice.description() + "\n";
    }
    QMessageBox::information(this, "Cameras", info);
}

void MainWindow::openCamera()
{
#ifdef GAZER_USE_QT_CAMERA
    camera->start();
#else
    if(capturer != nullptr) {
        capturer->setRunning(false);
        disconnect(capturer, &CaptureThread::frameCaptured, this, &MainWindow::updateFrame);
        connect(capturer, &CaptureThread::finished, capturer, &CaptureThread::deleteLater);
        disconnect(capturer, &CaptureThread::fpsChanged, this, &MainWindow::updateFPS);
        disconnect(capturer, &CaptureThread::videoSaved, this, &MainWindow::appendSavedVideo);
        disconnect(capturer, &CaptureThread::statusChange, this, &MainWindow::changeStatus);
    }

    int camID = 0;
    capturer = new CaptureThread(camID, data_lock);
    connect(capturer, &CaptureThread::frameCaptured, this, &MainWindow::updateFrame);
    connect(capturer, &CaptureThread::fpsChanged, this, &MainWindow::updateFPS);
    connect(capturer, &CaptureThread::videoSaved, this, &MainWindow::appendSavedVideo);
    connect(capturer, &CaptureThread::statusChange, this, &MainWindow::changeStatus);

    capturer->start();
    monitorCheckBox->setCheckState(Qt::Unchecked);
    mainStatusLabel->setText(QString("Capturing Camera %1").arg(camID));
    #endif
}


void MainWindow::updateFrame(cv::Mat *mat)
{
    data_lock->lock();
    currentFrame = *mat;
    data_lock->unlock();
    QImage frame(currentFrame.data,
                currentFrame.cols,
                currentFrame.rows,
                currentFrame.step,
    QImage::Format_RGB888);
    QPixmap image = QPixmap::fromImage(frame);
    imageScene->clear();
    imageView->resetTransform();
    imageScene->addPixmap(image);
    imageScene->update();
    imageView->setSceneRect(image.rect());
}

void MainWindow::calculateFPS()
{
     if(capturer != nullptr)
         capturer->startCalcFPS();
}

void MainWindow::updateFPS(float fps)
{
    mainStatusLabel->setText(QString("FPS of current camera is %1").arg(fps));
}

void MainWindow::recordingStartStop()
{
    QString text = recordButton->text();
    if(text == "Record" && capturer != nullptr) {
        capturer->setVideoSavingStatus(CaptureThread::STARTING);
        recordButton->setText("Stop Recording");
        monitorCheckBox->setCheckState(Qt::Unchecked);
        monitorCheckBox->setEnabled(false);
    } else if(text == "Stop Recording" && capturer != nullptr) {
        capturer->setVideoSavingStatus(CaptureThread::STOPPING);
        recordButton->setText("Record");
        monitorCheckBox->setEnabled(true);
    }
}

void MainWindow::appendSavedVideo(QString name)
{
    QString cover = Utilities::getSavedVideoPath(name, "jpg");
    QStandardItem *item = new QStandardItem();
    list_model->appendRow(item);
    QModelIndex index = list_model->indexFromItem(item);
    list_model->setData(index, QPixmap(cover).scaledToHeight(145), Qt::DecorationRole);
    list_model->setData(index, name, Qt::DisplayRole);
    saved_list->scrollTo(index);
}

void MainWindow::populateSavedList()
{
    QDir dir(Utilities::getDataPath());
    QStringList nameFilters;
    nameFilters << "*.jpg";
    QFileInfoList files = dir.entryInfoList(
        nameFilters, QDir::NoDotAndDotDot | QDir::Files, QDir::Name);

    foreach(QFileInfo cover, files) {
        QString name = cover.baseName();
        QStandardItem *item = new QStandardItem();
        list_model->appendRow(item);
        QModelIndex index = list_model->indexFromItem(item);
        list_model->setData(index, QPixmap(cover.absoluteFilePath()).scaledToHeight(145), Qt::DecorationRole);
        list_model->setData(index, name, Qt::DisplayRole);
    }
}

void MainWindow::updateMonitorStatus(int status)
{

    if(capturer == nullptr) {
        return;
    }
    if(status) {
        capturer->setMotionDetectingStatus(true);
        recordButton->setEnabled(false);
    } else {
        capturer->setMotionDetectingStatus(false);
        recordButton->setEnabled(true);
    }
}

void MainWindow::changeStatus(int fps, cv::Scalar mean,cv::Scalar dev)
{
    mainStatusLabel->setText(QString("FPS: %1 Mean: (%2.%3.%4) Dev: (%5.%6.%7)")
                             .arg(fps).arg(round(mean[0])).arg(round(mean[1])).arg(round(mean[2])).arg(round(dev[0])).arg(round(dev[1])).arg(round(dev[2])));
}
