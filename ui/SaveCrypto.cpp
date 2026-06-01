#include "SaveCrypto.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

namespace{

const QByteArray SaveMagic("BBOTDP01",8);

QByteArray jsonBytes(const QJsonObject& object){
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

bool parseJsonObject(const QByteArray& bytes,QJsonObject* object){
    QJsonParseError error;
    const QJsonDocument document=QJsonDocument::fromJson(bytes,&error);
    if(error.error!=QJsonParseError::NoError||!document.isObject()){
        return false;
    }
    if(object!=nullptr){
        *object=document.object();
    }
    return true;
}

#ifdef Q_OS_WIN
bool protectBytes(const QByteArray& plain,QByteArray* encrypted){
    DATA_BLOB input;
    input.pbData=reinterpret_cast<BYTE*>(const_cast<char*>(plain.constData()));
    input.cbData=static_cast<DWORD>(plain.size());
    DATA_BLOB output;
    if(!CryptProtectData(&input,L"BlockBot save",nullptr,nullptr,nullptr,0,&output)){
        return false;
    }
    *encrypted=QByteArray(reinterpret_cast<const char*>(output.pbData),
        static_cast<int>(output.cbData));
    LocalFree(output.pbData);
    return true;
}

bool unprotectBytes(const QByteArray& encrypted,QByteArray* plain){
    DATA_BLOB input;
    input.pbData=reinterpret_cast<BYTE*>(const_cast<char*>(encrypted.constData()));
    input.cbData=static_cast<DWORD>(encrypted.size());
    DATA_BLOB output;
    if(!CryptUnprotectData(&input,nullptr,nullptr,nullptr,nullptr,0,&output)){
        return false;
    }
    *plain=QByteArray(reinterpret_cast<const char*>(output.pbData),
        static_cast<int>(output.cbData));
    LocalFree(output.pbData);
    return true;
}
#else
bool protectBytes(const QByteArray& plain,QByteArray* encrypted){
    *encrypted=plain;
    return true;
}

bool unprotectBytes(const QByteArray& encrypted,QByteArray* plain){
    *plain=encrypted;
    return true;
}
#endif

bool readFileBytes(const QString& path,QByteArray* bytes){
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)){
        return false;
    }
    *bytes=file.readAll();
    return true;
}

}

namespace savecrypto{

QString appDataDirectoryPath(){
    QString dir=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if(dir.isEmpty()){
        dir=QCoreApplication::applicationDirPath();
    }
    QDir().mkpath(dir);
    return dir;
}

QString progressFilePath(){
    return QDir(appDataDirectoryPath()).filePath("data");
}

QString legacyProgressFilePath(){
    return QDir(appDataDirectoryPath()).filePath("level.json");
}

QString archiveDirectoryPath(){
    return appDataDirectoryPath();
}

QString cacheDirectoryPath(){
    QDir dir(appDataDirectoryPath());
    dir.mkpath("cache");
    return dir.filePath("cache");
}

bool writeEncryptedJsonFile(const QString& path,const QJsonObject& object){
    QByteArray encrypted;
    if(!protectBytes(jsonBytes(object),&encrypted)){
        return false;
    }
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly|QIODevice::Truncate)){
        return false;
    }
    file.write(SaveMagic);
    file.write(encrypted);
    return true;
}

bool readJsonFile(const QString& path,QJsonObject* object){
    QByteArray bytes;
    if(!readFileBytes(path,&bytes)){
        return false;
    }
    if(bytes.startsWith(SaveMagic)){
        QByteArray plain;
        if(!unprotectBytes(bytes.mid(SaveMagic.size()),&plain)){
            return false;
        }
        return parseJsonObject(plain,object);
    }
    return parseJsonObject(bytes,object);
}

bool progressSaveExists(){
    return QFileInfo::exists(progressFilePath())||
           QFileInfo::exists(legacyProgressFilePath());
}

bool writeProgressLevel(int level){
    QJsonObject object;
    object["level"]=level;
    return writeEncryptedJsonFile(progressFilePath(),object);
}

int readProgressLevel(int fallback){
    QJsonObject object;
    if(readJsonFile(progressFilePath(),&object)){
        return object.value("level").toInt(fallback);
    }
    if(readJsonFile(legacyProgressFilePath(),&object)){
        return object.value("level").toInt(fallback);
    }
    return fallback;
}

}
