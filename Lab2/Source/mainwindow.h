#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QGraphicsView>
#include <QRegularExpression>
#include <QApplication>
#include <QFileDialog>
#include <QStatusBar>
#include <QLabel>
#include <QGraphicsPixmapItem>
#include <QShortcut>


#include <QMap>
#include <QPluginLoader>
#include "editor_plugin_interface.h"
#include "opencv2/opencv.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initUI();
    void setupShortcuts();
    void showImage(QString path);
    bool isLastImgInPath();
    bool isFirstImgInPath();
    void loadPlugins();

private slots:
    void openImage();
    void zoomIn();
    void zoomOut();
    void saveAs();
    void previousImg();
    void nextImg();
    void rotateImg();
    void cancelImg();

    void blurImage();
    void pluginPerform();
private:
    QMap<QString, EditorPluginInterface*> editPlugins;

    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *editMenu;

    QToolBar *fileToolBar;
    QToolBar *viewToolBar;
    QToolBar *editToolBar;

    QStatusBar *mainStatusBar;
    QLabel *mainStatusLabel;

    QAction *openAction;
    QAction *SaveAsAction;
    QAction *exitAction;
    QAction *zoomInAction;
    QAction *zoomOutAction;
    QAction *previousImgAction;
    QAction *nextImgAction;
    QAction *rotateAction;
    QAction *cancelAction;
    QAction *blurAction;

    QGraphicsScene *imageScene;
    QGraphicsView *imageView;
    QGraphicsPixmapItem *currentImage;

    QString currentImagePath;

};

#endif // MAINWINDOW_H
