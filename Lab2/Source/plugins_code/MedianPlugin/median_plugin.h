#ifndef MEDIAN_PLUGIN_H
#define MEDIAN_PLUGIN_H
#include <QObject>
#include <QtPlugin>
#include "editor_plugin_interface.h"

class median_plugin: public QObject, public EditorPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID EDIT_PLUGIN_INTERFACE_IID);
    Q_INTERFACES(EditorPluginInterface);
public:
    QString name();
    void edit(const cv::Mat &input, cv::Mat &output);
};

#endif // MEDIAN_PLUGIN_H
