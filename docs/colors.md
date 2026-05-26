# 积木颜色整理

本文记录当前代码中的积木与相关 UI 颜色，来源主要是 `ui/App.cpp` 和 `ui/Widgets.h`。

## CodeBlock 类积木

| 类别 | 颜色 | RGB | 位置 |
| --- | --- | --- | --- |
| 普通代码积木默认色 | 蓝色 | `Qt::blue` | `CodeBlock` |
| 输出积木 | 紫色 | `QColor(130, 76, 180)` | `OutputBlock` |
| 变量赋值积木 | 橙色 | `QColor(194, 92, 0)` | `SetVariableBlock` |
| 列表操作积木 | 深红色 | `QColor(125, 28, 38)` | `PushListBlock`, `SetListBlock`, `ClearListBlock` |
| 控制积木 | 蓝灰色 | `QColor(54, 92, 122)` | `ControlCodeBlock` |
| 开始积木 | 棕黄色 | `QColor(156, 118, 42)` | `StartBlock` |
| 自定义积木 | 深紫色 | `QColor(82, 45, 122)` | `CustomHatBlock`, `CustomCallBlock` |

## FloatBlock 类积木

| 类别 | 颜色 | RGB | 位置 |
| --- | --- | --- | --- |
| 普通数值块 | 灰蓝色 | `QColor(92, 102, 116)` | `FloatBlock` |
| 变量读取块 | 橙色 | `QColor(194, 92, 0)` | `VariableBlock` |
| 运算块 | 深绿色 | `QColor(42, 105, 86)` | `UnaryOpBlock`, `BinaryOpBlock` |
| 列表读取 / 列表长度块 | 深红色 | `QColor(125, 28, 38)` | `ListGetBlock`, `ListSizeBlock` |
| 自定义参数块 | 深紫色 | `QColor(82, 45, 122)` | `CustomParamBlock` |
| 机器人坐标 / 前方类型块 | 深蓝色 | `QColor(42, 86, 150)` | `RobotCoordBlock`, `RobotFrontMapBlock` |

## 可编辑名称框

| 类别 | 颜色 | RGB | 位置 |
| --- | --- | --- | --- |
| 变量名 / 列表名 / 输出文字 / 自定义参数名背景 | 浅灰色 | `QColor(220, 220, 220)` | `variableFrame`, `listFrame`, `messageFrame`, `parameterFrame` |
| 名称文字 | 黑色 | `Qt::black` | `ClickTextItem` |

## 按钮与辅助 UI

| 类别 | 颜色 | RGB | 位置 |
| --- | --- | --- | --- |
| 普通文字按钮 | 蓝色 | `QColor(80, 120, 170)` | `TextButton` |
| 右键菜单按钮 | 深灰蓝 | `QColor(70, 80, 96)` | `ContextMenuButton` |
| 测试按钮 | 深灰蓝 | `QColor(70, 80, 96)` | `testButton` |
| 文件按钮 | 紫色 | `QColor(126, 70, 180)` | 保存 / 打开 / 退出 |
| 创建变量按钮 | 橙色 | `QColor(194, 92, 0)` | `createVariableButton` |
| 创建列表按钮 | 深红色 | `QColor(125, 28, 38)` | `createListButton` |
| 创建自定义积木按钮 | 深紫色 | `QColor(82, 45, 122)` | `createCustomBlockButton` |
| 吸附高亮边框 | 黄色 | `QColor(255, 210, 0)` | `absorbShadow` |

## 地图显示颜色

| 类别 | 颜色 | RGB | 位置 |
| --- | --- | --- | --- |
| 舞台背景 | 白色 | `Qt::white` | `stage` |
| 普通格子 | 灰色 | `Qt::gray` | `CellEmpty` |
| 墙 | 棕色 | `QColor(126, 76, 36)` | `CellWall` |
| 陷阱 | 红色 | `QColor(178, 48, 48)` | `CellTrap` |
| 机器人 | 绿色 | `Qt::green` | `Robot` |

## 基础按钮图形颜色

`ui/Widgets.h` 中 `Button` 图形根据类型设置颜色：

| 类型 | 颜色 | RGB |
| --- | --- | --- |
| 默认按钮 | 蓝色 | `QColor(80, 140, 235)` |
| 类型 0 | 绿色 | `QColor(44, 135, 82)` |
| 类型 1 | 金黄色 | `QColor(202, 150, 36)` |
| 类型 2 | 红色 | `QColor(180, 48, 48)` |
| 滚动条滑块 | 深灰色 | `QColor(75, 85, 99)` |

