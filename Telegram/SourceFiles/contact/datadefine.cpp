#include "datadefine.h"
#include <QFile>

namespace Contact {

    QString getAllFileContent(const QString& path)
    {
        QFile file(path);
        file.open(QFile::ReadOnly);
        QString skinInfo = file.readAll();
        file.close();
        return skinInfo;
    }
}