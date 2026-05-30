#ifndef ARCHIVE_ARCHIVEPAGE_H
#define ARCHIVE_ARCHIVEPAGE_H

#include <QDialog>

class QLabel;

class ArchivePage:public QDialog{
    Q_OBJECT
public:
    explicit ArchivePage(QWidget* parent=nullptr);
    void init();

signals:
    void pageClosed();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QLabel* contentLabel=nullptr;
};

#endif
