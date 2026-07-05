# BlockBot:Factory

BlockBot:Factory是一个基于Qt6和C++17的图形化编程解谜游戏。玩家通过拖拽积木编写程序,控制机器人R-07在工厂关卡中移动、等待、读取传感信息,并完成移动、排序、解密、压力分析和多目标结算等任务。

[网盘链接](https://disk.pku.edu.cn/anyshare/zh-cn/link/AAEF4A735F716B48A69C353EA422B03768/13849E329F2E4EAAA51A5E1C84083D9D/BAFCC8A205084346A5C3597F392D1AB2)

## 主要结构

- `main.cpp`:程序入口,调用`ui::runApp(argc,argv)`启动Qt主界面。
- `CMakeLists.txt`:CMake构建入口,生成目标为`BlockBot`,使用Qt6 Widgets和Qt6 Multimedia,Windows下链接`Crypt32`。
- `core/`:运行时状态、变量和列表、值对象、表达式运算、积木执行相关代码。
- `level/`:关卡配置、地图数据、关卡类型、目标测试和迷宫生成逻辑。
- `ui/`:主窗口、关卡选择、沙盒和关卡编辑界面、档案页、设置页、存档加密和音频管理。
- `tale/`:剧情窗口、剧情段落和角色立绘显示逻辑。
- `message/`:运行错误和提示弹窗入口。
- `docs/`:积木说明、提示、剧情和存档相关文档。
- `ans/`:九个关卡的参考答案JSON文件。
- `images/`:背景图、地块贴图、人物立绘、按钮和图标资源。
- `music/`:背景音乐和交互音效。
- `resource.qrc`:Qt资源清单,由`CMakeLists.txt`通过`qt_add_big_resources`打包。

## 展示材料

- `demo.mp4`:项目展示视频之一。
- `narration.mp4`:项目展示视频之一。
- `report.html`:项目报告网页。
- `report.pdf`:由`report.html`导出的PDF报告。

## 运行方法

[网盘链接](https://disk.pku.edu.cn/anyshare/zh-cn/link/AAEF4A735F716B48A69C353EA422B03768/13849E329F2E4EAAA51A5E1C84083D9D/BAFCC8A205084346A5C3597F392D1AB2)

目前按Windows环境说明。需要先安装Qt6、CMake和可用的C++17编译器,Qt组件至少包括Widgets和Multimedia。

使用CMake构建:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.11.0\mingw_64
cmake --build build --config Release
```

运行程序:

```powershell
.\build\BlockBot.exe
```

如果使用Visual Studio等多配置生成器,可执行文件通常位于:

```powershell
.\build\Release\BlockBot.exe
```

也可以用Qt Creator打开`CMakeLists.txt`,选择Qt6 Kit后直接构建并运行。
