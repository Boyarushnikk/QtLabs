#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    initUI();
    setupShortcuts();
    loadPlugins();
}

MainWindow::~MainWindow()
{
}

void MainWindow::initUI()
{
    resize(800, 600);
     // setup menubar
    fileMenu = menuBar()->addMenu("&File");
    viewMenu = menuBar()->addMenu("&View");
    editMenu = menuBar()->addMenu("&Edit");

      // setup toolbar
    fileToolBar = addToolBar("File");
    viewToolBar = addToolBar("View");
    editToolBar = addToolBar("Edit");


      // main area for image display
    imageScene = new QGraphicsScene(this);
    imageView = new QGraphicsView(imageScene);
    setCentralWidget(imageView);

      // setup status bar
    mainStatusBar = statusBar();
    mainStatusLabel = new QLabel("mainStatusBar");
    mainStatusBar->addPermanentWidget(mainStatusLabel);
    mainStatusLabel->setText("Image Information will be here!");

    openAction = new QAction(QIcon(":/icons/icons/open.png"), "&Open", this);
    connect(openAction, SIGNAL(triggered(bool)), this, SLOT(openImage()));
    fileMenu->addAction(openAction);
    fileToolBar->addAction(openAction);

    SaveAsAction = new QAction(QIcon(":/icons/icons/save.png"), "&Save as", this);
    connect(SaveAsAction, SIGNAL(triggered(bool)), this, SLOT(saveAs()));
    fileMenu->addAction(SaveAsAction);

    exitAction = new QAction(QIcon(":/icons/icons/exit.png"), "&Exit", this);
    connect(exitAction, SIGNAL(triggered(bool)), QApplication::instance(), SLOT(quit()));
    fileMenu->addAction(exitAction);

    zoomInAction = new QAction(QIcon(":/icons/icons/plus.png"), "&Zoom In", this);
    connect(zoomInAction, SIGNAL(triggered(bool)), this, SLOT(zoomIn()));
    viewMenu->addAction(zoomInAction);
    viewToolBar->addAction(zoomInAction);

    zoomOutAction = new QAction(QIcon(":/icons/icons/minus.png"), "&Zoom Out", this);
    connect(zoomOutAction, SIGNAL(triggered(bool)), this, SLOT(zoomOut()));
    viewMenu->addAction(zoomOutAction);
    viewToolBar->addAction(zoomOutAction);

    previousImgAction = new QAction(QIcon(":/icons/icons/previous.png"), "&Previous Image", this);
    connect(previousImgAction, SIGNAL(triggered(bool)), this, SLOT(previousImg()));
    viewMenu->addAction(previousImgAction);
    viewToolBar->addAction(previousImgAction);

    nextImgAction = new QAction(QIcon(":/icons/icons/next.png"), "&Next Image", this);
    connect(nextImgAction, SIGNAL(triggered(bool)), this, SLOT(nextImg()));
    viewMenu->addAction(nextImgAction);
    viewToolBar->addAction(nextImgAction);

    rotateAction = new QAction(QIcon(":/icons/icons/rotate.png"), "&Rotate Image", this);
    connect(rotateAction, SIGNAL(triggered(bool)), this, SLOT(rotateImg()));
    viewMenu->addAction(rotateAction);
    viewToolBar->addAction(rotateAction);

    cancelAction = new QAction("&Cancel", this);
    connect(cancelAction, SIGNAL(triggered(bool)), this, SLOT(cancelImg()));
    viewMenu->addAction(cancelAction);
    viewToolBar->addAction(cancelAction);

    blurAction = new QAction("&Blure", this);
    connect(blurAction, SIGNAL(triggered(bool)), this, SLOT(blurImage()));
    editMenu->addAction(blurAction);
    editToolBar->addAction(blurAction);
}

void MainWindow::setupShortcuts()
{
    QList<QKeySequence> shortcuts;
    shortcuts << Qt::Key_Plus << Qt::Key_Equal;
    zoomInAction->setShortcuts(shortcuts);
    shortcuts.clear();
    shortcuts << Qt::Key_Minus << Qt::Key_Underscore;
    zoomOutAction->setShortcuts(shortcuts);
    shortcuts.clear();
    shortcuts << Qt::Key_Up << Qt::Key_Left;
    previousImgAction->setShortcuts(shortcuts);
    shortcuts.clear();
    shortcuts << Qt::Key_Down << Qt::Key_Right;
    nextImgAction->setShortcuts(shortcuts);
}

void MainWindow::showImage(QString path)
{
    currentImagePath = path;
    imageScene->clear();
    imageView->resetTransform();
    QPixmap image(path);
    currentImage = imageScene->addPixmap(image);
    imageScene->update();
    imageView->setSceneRect(image.rect());
    QString status = QString("%1, %2x%3, %4 Bytes").arg(path).arg(image.width())
    .arg(image.height()).arg(QFile(path).size());
    mainStatusLabel->setText(status);
    previousImgAction->setEnabled(!isFirstImgInPath());
    nextImgAction->setEnabled(!isLastImgInPath());
}

bool MainWindow::isLastImgInPath()
{
    QFileInfo current(currentImagePath);
    QDir dir = current.absoluteDir();
    QStringList nameFilters;
    nameFilters << "*.png" << "*.bmp" << "*.jpg";
    QStringList fileNames = dir.entryList(nameFilters, QDir::Files, QDir::Name);
    int idx = fileNames.indexOf(QRegularExpression(QRegularExpression::escape(current.fileName())));
    if(idx == fileNames.length() - 1)
        return true;
    return false;
}

bool MainWindow::isFirstImgInPath()
{
    QFileInfo current(currentImagePath);
    QDir dir = current.absoluteDir();
    QStringList nameFilters;
    nameFilters << "*.png" << "*.bmp" << "*.jpg";
    QStringList fileNames = dir.entryList(nameFilters, QDir::Files, QDir::Name);
    int idx = fileNames.indexOf(QRegularExpression(QRegularExpression::escape(current.fileName())));
    if (idx == 0)
        return true;
    return false;
}

void MainWindow::loadPlugins()
{
    QDir pluginsDir(QApplication::instance()->applicationDirPath() + "/plugins");
    QStringList nameFilters;
    nameFilters << "*.so" << "*.dylib" << "*.dll";
    QFileInfoList plugins = pluginsDir.entryInfoList(
                    nameFilters, QDir::NoDotAndDotDot | QDir::Files, QDir::Name);
    foreach(QFileInfo plugin, plugins) {
        QPluginLoader pluginLoader(plugin.absoluteFilePath(), this);
        EditorPluginInterface *plugin_ptr = dynamic_cast<EditorPluginInterface*>(pluginLoader.instance());
        if(plugin_ptr) {
            QAction *action = new QAction(plugin_ptr->name());

            if(plugin_ptr->name()=="Erode")
                action->setShortcut(Qt::Key_E);
            if(plugin_ptr->name()=="Median")
                action->setShortcut(Qt::Key_M);

            editMenu->addAction(action);
            editToolBar->addAction(action);
            editPlugins[plugin_ptr->name()] = plugin_ptr;
            connect(action, SIGNAL(triggered(bool)), this, SLOT(pluginPerform()));
            //pluginLoader.unload();
        } else {
            qDebug() << "bad plugin: " << plugin.absoluteFilePath();
        }
    }
}

void MainWindow::openImage()
{
    QFileDialog dialog(this);
    dialog.setWindowTitle("Open Image");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Images (*.png *.bmp *.jpg)"));
    QStringList filePaths;
    if (dialog.exec()) {
        filePaths = dialog.selectedFiles();
        showImage(filePaths.at(0));
    }
}

void MainWindow::zoomIn()
{
    imageView->scale(1.2, 1.2);
}

void MainWindow::zoomOut()
{
    imageView->scale(1/1.2, 1/1.2);
}

void MainWindow::saveAs()
{
    if (currentImage == nullptr) {
     QMessageBox::information(this, "Information", "Nothing to save.");
     return;
     }
    QFileDialog dialog(this);
    dialog.setWindowTitle("Save Image As ...");
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(tr("Images (*.png *.bmp *.jpg)"));
    QStringList fileNames;
    if (dialog.exec()) {
       fileNames = dialog.selectedFiles();
       QRegularExpression version(".+\\.(png|bmp|jpg)");
       QRegularExpressionMatch match = version.match(fileNames.at(0));
       if(match.hasMatch()) {
           currentImage->pixmap().save(fileNames.at(0));
       } else {
           QMessageBox::information(this, "Information", "Save error: bad format or filename.");
       }
    }
}

void MainWindow::previousImg()
{
    QFileInfo current(currentImagePath);
    QDir dir = current.absoluteDir();
    QStringList nameFilters;
    nameFilters << "*.png" << "*.bmp" << "*.jpg";
    QStringList fileNames = dir.entryList(nameFilters, QDir::Files, QDir::Name);
    int idx = fileNames.indexOf(QRegularExpression(QRegularExpression::escape(current.fileName())));
    if(idx > 0) {
        showImage(dir.absoluteFilePath(fileNames.at(idx - 1)));
    } else {
        QMessageBox::information(this, "Information", "Current image is the first one.");
    }
}

void MainWindow::nextImg()
{
    QFileInfo current(currentImagePath);
    QDir dir = current.absoluteDir();
    QStringList nameFilters;
    nameFilters << "*.png" << "*.bmp" << "*.jpg";
    QStringList fileNames = dir.entryList(nameFilters, QDir::Files, QDir::Name);
    int idx = fileNames.indexOf(QRegularExpression(QRegularExpression::escape(current.fileName())));
    if(idx <= fileNames.length()-2) {
        showImage(dir.absoluteFilePath(fileNames.at(idx + 1)));
    } else {
        QMessageBox::information(this, "Information", "Current image is the last one.");
    }
}

void MainWindow::rotateImg()
{
    imageView->rotate(90);
}

void MainWindow::cancelImg()
{
    showImage(currentImagePath);
}

void MainWindow::blurImage()
{
    if (currentImage == nullptr) {
            QMessageBox::information(this, "Information", "No image to edit.");
            return;
        }

        QPixmap pixmap = currentImage->pixmap();
        QImage image = pixmap.toImage();

        image = image.convertToFormat(QImage::Format_RGB888);
        cv::Mat mat = cv::Mat(image.height(),
                              image.width(),
                              CV_8UC3,
                              image.bits(),
                              image.bytesPerLine());

        cv::Mat tmp;
        cv::blur(mat, tmp, cv::Size(8, 8));
        mat = tmp;

        QImage image_blurred(mat.data,
                            mat.cols,
                            mat.rows,
                            mat.step,
                            QImage::Format_RGB888);
        pixmap = QPixmap::fromImage(image_blurred);
        imageScene->clear();
        imageView->resetTransform();
        currentImage = imageScene->addPixmap(pixmap);
        imageScene->update();
        imageView->setSceneRect(pixmap.rect());

        QString status = QString("(editted image), %1x%2")
                          .arg(pixmap.width()).arg(pixmap.height());
        mainStatusLabel->setText(status);
}

void MainWindow::pluginPerform()
{
    if (currentImage == nullptr) {
         QMessageBox::information(this, "Information", "No image to edit.");
         return;
        }

        QAction *active_action = qobject_cast<QAction*>(sender());
        EditorPluginInterface *plugin_ptr = editPlugins[active_action->text()];
        if(!plugin_ptr) {
            QMessageBox::information(this, "Information", "No plugin is found.");
            return;
        }

        QPixmap pixmap = currentImage->pixmap();
        QImage image = pixmap.toImage();
        image = image.convertToFormat(QImage::Format_RGB888);
        cv::Mat mat = cv::Mat(image.height(),
                    image.width(),
                    CV_8UC3,
                    image.bits(),
                    image.bytesPerLine());
        plugin_ptr->edit(mat, mat);
        QImage image_edited(mat.data,
                            mat.cols,
                            mat.rows,
                            mat.step,
                            QImage::Format_RGB888);
        pixmap = QPixmap::fromImage(image_edited);
        imageScene->clear();
        imageView->resetTransform();
        currentImage = imageScene->addPixmap(pixmap);
        imageScene->update();
        imageView->setSceneRect(pixmap.rect());
        QString status = QString("(editted image), %1x%2")
                .arg(pixmap.width()).arg(pixmap.height());
        mainStatusLabel->setText(status);
}

