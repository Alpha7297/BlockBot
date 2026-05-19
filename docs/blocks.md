# 积木整理表

## 程序入口

start 整个程序的开始，不允许多线程

f(x) 自定义函数的开始，不允许重名

## 浮点数积木

构造函数顺序
内部参数，类型

### 浮点数一元运算符

使用角度制
sin 0
cos 1
tan 2
asin返回值$[-\frac{\pi}{2},\frac{\pi}{2}]$ 3
acos返回值$[0,\pi]$ 4
atan返回值$[-\frac{\pi}{2},\frac{\pi}{2}]$ 5
$\ln$ 6
$\log_{10}$ 7
floor 8
abs 9

### 浮点数二元运算符

add 0
minus 1
mul 2
div 3
pow 4
arg 5
max 6
min 7

## 布尔数积木

### 常数

true false

### 布尔数一元运算符

! 0

### 布尔数二元运算符

== 0
< 1
\> 2
!= 3

&& 4
|| 5

## 交互积木

构造函数顺序
before next type 内部参数

### 基础积木

零元
turnleft 0
turnright 1
单元
move x 0
wait x 1
二元
set x y 0，将x变量值设定为y

### 逻辑积木

if 0
while 1

## 变量积木

只支持double 类型

单元操作符
get x返回x对应内容
set x y将变量x设置为y，命名需要完全一致
