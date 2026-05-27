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
    showRuntimeError("禁止除以0");
}

void inverseTrigOutOfRange(const char* op,double value){
    showRuntimeError(
        QString("%1的参数必须在-1到1之间，当前值是%2.")
            .arg(QString::fromUtf8(op))
            .arg(value,0,'g',6)
    );
}

void numericOutOfRange(const char* op){
    showRuntimeError(
        QString("数值结果超过上限%1.数值截断到上限")
            .arg(QString::fromUtf8(op))
    );
}

void multithreadingNotAllowed(){
    showRuntimeError("禁止多线程运行，只能存在一个start");
}

void variableNotFound(const char* name){
    showRuntimeError(
        QString("变量\"%1\"不存在，运行终止。")
            .arg(QString::fromUtf8(name))
    );
}

void listNotFound(const char* name){
    showRuntimeError(
        QString("列表\"%1\"不存在，运行结束。")
            .arg(QString::fromUtf8(name))
    );
}

void readOnlyValue(const char* name){
    showRuntimeError(
        QString("\"%1\" 这一关只已读，禁止修改。运行终止。")
            .arg(QString::fromUtf8(name))
    );
}

void listIndexOutOfRange(const char* name,int index){
    showRuntimeError(
        QString("列表 \"%1\" 索引 %2 超过范围. 运行终止。")
            .arg(QString::fromUtf8(name))
            .arg(index)
    );
}

void invalidVariableName(){
    showRuntimeError("名称可以使用中文、英文、数字和下划线，不能有空格，最多 10 个字符，禁止与已有重复");
}

void workspaceWidthLimitReached(){
    showRuntimeError("这个积木长度超过工作区上限，已移除");
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
