#ifndef MESSAGE_MESSAGE_H
#define MESSAGE_MESSAGE_H

namespace message{

void divisionByZero();
void inverseTrigOutOfRange(const char* op,double value);
void numericOutOfRange(const char* op);
void multithreadingNotAllowed();
void variableNotFound(const char* name);
void listNotFound(const char* name);
void readOnlyValue(const char* name);
void listIndexOutOfRange(const char* name,int index);
void invalidVariableName();
void workspaceWidthLimitReached();
void singleDialogOnly();
void runtimeError(const char* text);

}

#endif
