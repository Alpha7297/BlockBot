#include "Message.h"

#include <QMessageBox>
#include <QString>
#include <string>

namespace message{

namespace{

bool dialogActive=false;

void showRuntimeError(const QString& text){
    if(dialogActive){
        return;
    }
    dialogActive=true;
    QMessageBox::warning(nullptr,"Runtime Error",text);
    dialogActive=false;
}

void showOutput(const QString& text){
    if(dialogActive){
        return;
    }
    dialogActive=true;
    QMessageBox::information(nullptr,"输出",text);
    dialogActive=false;
}

}

void runtimeError(const char* text){
    showRuntimeError(QString::fromUtf8(text));
}

void divisionByZero(){
    showRuntimeError("Division by zero is not allowed.");
}

void inverseTrigOutOfRange(const char* op,double value){
    showRuntimeError(
        QString("%1 input must be between -1 and 1. Current value: %2.")
            .arg(QString::fromUtf8(op))
            .arg(value,0,'g',6)
    );
}

void numericOutOfRange(const char* op){
    showRuntimeError(
        QString("Numeric result is out of range in %1. The value was clamped to the double limit.")
            .arg(QString::fromUtf8(op))
    );
}

void multithreadingNotAllowed(){
    showRuntimeError("Multithreading is not allowed. Only one start block can exist in the workspace.");
}

void variableNotFound(const char* name){
    showRuntimeError(
        QString("Variable \"%1\" does not exist. Program execution was stopped.")
            .arg(QString::fromUtf8(name))
    );
}

void listNotFound(const char* name){
    showRuntimeError(
        QString("List \"%1\" does not exist. Program execution was stopped.")
            .arg(QString::fromUtf8(name))
    );
}

void readOnlyValue(const char* name){
    showRuntimeError(
        QString("\"%1\" is read-only in this level. Program execution was stopped.")
            .arg(QString::fromUtf8(name))
    );
}

void listIndexOutOfRange(const char* name,int index){
    showRuntimeError(
        QString("List \"%1\" index %2 is out of range. Program execution was stopped.")
            .arg(QString::fromUtf8(name))
            .arg(index)
    );
}

void invalidVariableName(){
    showRuntimeError("名称可以使用中文、英文、数字和下划线，不能有空格，最多 10 个字符。");
}

void workspaceWidthLimitReached(){
    showRuntimeError("The block is too wide for the workspace. It was removed.");
}

void singleDialogOnly(){
    showRuntimeError("Only one runtime error dialog can be shown at a time.");
}
void otherError(const char* text)
{
    showRuntimeError(text);
}
void otherError(const std::string text)
{
    showRuntimeError(text.c_str());
}

void output(const char* text)
{
    showOutput(QString::fromUtf8(text));
}

void output(const std::string text)
{
    showOutput(QString::fromUtf8(text.c_str()));
}

}
