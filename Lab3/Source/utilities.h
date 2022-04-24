#ifndef UTILITIES_H
#define UTILITIES_H

#include <QString>
#include <QDir>
#include <QStandardPaths>

class Utilities
{
public:
    static QString getDataPath();
    static QString newSavedVideoName();
    static QString getSavedVideoPath(QString name, QString postfix);
};

#endif // UTILITIES_H
