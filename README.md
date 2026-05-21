# BotBlocks: Odyssey

## 游戏内容

包含几个页面

标题页面：
关卡模式，沙盒模式两个button，加上Logo

关卡选择页面：
选择关卡方式为点线式连接，通过一关后从该点向外连接线连接到下一关，有相关特效
点击某一关进入

沙盒模式：
普通编辑页面，没有挑战按钮

关卡模式：
每一关初始地图不同，完成任务不同，所能使用的积木块也不相同

故事页面：
内嵌在关卡模式上方，人物进行对话，对话框+文字形式，说明这一关的任务，对话结束之后消失

## 关卡设计

1. 入门系列
引导用户学习如何使用这款语言
1.1 空白地图，编写代码，让机器人移动到指定位置
1.2 空白地图，加上障碍和陷阱，让机器人移动到指定位置
1.3 学习变量——让机器人走正方形
1.4 挑战迷宫，简单版
1.5 挑战迷宫，噩梦版

2. 算法系列
引导用户完成算法设计
2.1 学习列表——给定一个列表，输出列表的和
2.2 学习排序——给定一个列表，设计算法从小到大排序
2.3 学习自定义函数——设计算法解决
2.3 长方形——给定一个整数列表，每个块代表这一块的高度，找到最大的长方形

3. 对战系列
加入新的积木块进行攻击，具体逻辑还没想好

## 编译运行

本仓库不需要提交 `build`、`build-run` 或 `run.bat`。每个人在自己的电脑上用 CMake 重新生成构建目录即可。

### Windows

如果 Qt 已经加入环境变量：

```powershell
cmake -S . -B build
cmake --build build
.\build\helloQt.exe
```

如果 Qt 没有加入环境变量，需要显式指定 Qt 的安装位置，例如：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.0\mingw_64"
cmake --build build
.\build\helloQt.exe
```

测试 core 计算逻辑：

```powershell
cmake --build build --target test_calc
.\build\test_calc.exe
```

### Linux

如果系统已经安装 Qt6：

```bash
cmake -S . -B build
cmake --build build
./build/helloQt
```

如果 Qt 安装在自定义目录：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build
./build/helloQt
```

测试 core 计算逻辑：

```bash
cmake --build build --target test_calc
./build/test_calc
```
