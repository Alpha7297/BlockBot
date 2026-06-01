#ifndef UI_SAVECRYPTO_H
#define UI_SAVECRYPTO_H

#include <QJsonObject>
#include <QString>

namespace savecrypto{

QString appDataDirectoryPath();
QString progressFilePath();
QString legacyProgressFilePath();
QString archiveDirectoryPath();
QString cacheDirectoryPath();

bool writeEncryptedJsonFile(const QString& path,const QJsonObject& object);
bool readJsonFile(const QString& path,QJsonObject* object);
bool progressSaveExists();
bool writeProgressLevel(int level);
int readProgressLevel(int fallback=-1);

}

#endif
