# 积木整理

本文档记录 `ui/App.cpp` 中当前所有积木类的类型、父类、颜色和初始尺寸。

代码里大多数积木使用两个字段描述尺寸：

- `len`：横向长度。
- `wid`：纵向宽度，也就是视觉高度。

所以下表中的尺寸统一写作 `len × wid`。

## 基础尺寸常量

| 名称 | 数值 | 说明 |
| --- | --- | --- |
| `floatBlockWidth` | `25` | 数值类积木的默认高度，也是空数值块的最小长度 |
| `variableHorizontalPadding` | `6` | 变量名、列表名、参数名两侧的横向内边距 |
| `CodeBlock` 默认高度 | `40` | 普通语句积木默认 `wid` |
| 名称编辑框颜色 | `QColor(220,220,220)` | 变量名、列表名、输出文字、自定义参数名的输入框背景 |

## 语句类积木

| 积木 | 类 | 父类 | `type` | 颜色 | 初始尺寸 |
| --- | --- | --- | --- | --- | --- |
| 普通语句积木 | `CodeBlock` | `QGraphicsPolygonItem` | 由构造参数决定 | `Qt::blue` | `(文字宽度 + 30) × 40` |
| 左转 | `CodeBlock` | `QGraphicsPolygonItem` | `0` | `Qt::blue` | `(文字宽度 + 30) × 40` |
| 右转 | `CodeBlock` | `QGraphicsPolygonItem` | `1` | `Qt::blue` | `(文字宽度 + 30) × 40` |
| 向前移动 | `CodeBlock` | `QGraphicsPolygonItem` | `3` | `Qt::blue` | `(文字宽度 + 30) × 40` |
| 等待 | `FloatCodeBlock` | `CodeBlock` | `4` | 继承 `CodeBlock` 的 `Qt::blue` | `ceil(20 + 文字宽度 + 10 + 数值长度 + 10 + 后缀宽度 + 10) × max(40, 数值高度 + 10)` |
| 输出 | `OutputBlock` | `CodeBlock` | `12` | `QColor(130,76,180)` | `ceil(20 + 文字宽度 + 10 + 文本框长度 + 10 + 数值长度 + 10) × max(40, 数值高度 + 10)` |
| 将变量设置为数值 | `SetVariableBlock` | `CodeBlock` | `7` | `QColor(194,92,0)` | `ceil(20 + 文字宽度 + 10 + 变量框长度 + 10 + 后缀宽度 + 10 + 数值长度 + 10) × max(40, 数值高度 + 10)` |
| 在列表末尾加入 | `PushListBlock` | `CodeBlock` | `8` | `QColor(125,28,38)` | `ceil(20 + 文字宽度 + 10 + 列表框长度 + 10 + 后缀宽度 + 10 + 数值长度 + 10) × max(40, 数值高度 + 10)` |
| 设置列表某项 | `SetListBlock` | `CodeBlock` | `9` | `QColor(125,28,38)` | `ceil(20 + 文字宽度 + 10 + 列表框长度 + 10 + 中间文字宽度 + 10 + 下标长度 + 10 + 后缀宽度 + 10 + 数值长度 + 10) × max(40, max(下标高度, 数值高度) + 10)` |
| 清空列表 | `ClearListBlock` | `CodeBlock` | `10` | `QColor(125,28,38)` | `ceil(20 + 文字宽度 + 10 + 列表框长度 + 10) × 40` |
| 自定义积木调用 | `CustomCallBlock` | `CodeBlock` | `11` | `QColor(82,45,122)` | 无参数：`(文字宽度 + 30) × 40`；有参数：`ceil(20 + 文字宽度 + 10 + 参数长度 + 10) × max(40, 参数高度 + 10)` |

## 控制和入口积木

| 积木 | 类 | 父类 | `type` | 颜色 | 初始尺寸 |
| --- | --- | --- | --- | --- | --- |
| 开始运行 | `StartBlock` | `CodeBlock` | `-1` | `QColor(156,118,42)` | `(文字宽度 + 30) × 40` |
| 如果 | `ControlCodeBlock` | `CodeBlock` | `5` | `QColor(54,92,122)` | `max(90, 20 + 文字宽度 + 10 + 条件长度 + 10 + 后缀宽度 + 10) × (topHeight + innerHeight + bottomHeight)` |
| 当 / while | `ControlCodeBlock` | `CodeBlock` | `6` | `QColor(54,92,122)` | 同 `ControlCodeBlock` |
| 自定义积木帽子 | `CustomHatBlock` | `CodeBlock` | `-3` | `QColor(82,45,122)` | 无参数：`ceil(文字宽度 + 30) × 40`；有参数：`ceil(20 + 文字宽度 + 10 + 参数名框长度 + 10 + 参数块长度 + 10) × max(40, 参数块高度 + 10)` |

`ControlCodeBlock` 的默认结构尺寸：

| 字段 | 默认值 | 说明 |
| --- | --- | --- |
| `topHeight` | `30` | 条件所在的顶部高度，会根据条件积木高度增大 |
| `innerHeight` | `30` | 内部链条区域高度，会根据内部积木链条增大 |
| `bottomHeight` | `20` | 底部封口高度 |
| `leftWidth` | `10` | 左侧包裹区域宽度 |

## 数值类积木

| 积木 | 类 | 父类 | `type` | 颜色 | 初始尺寸 |
| --- | --- | --- | --- | --- | --- |
| 普通数值 | `FloatBlock` | `QGraphicsPolygonItem` | 由构造参数决定 | `QColor(92,102,116)` | `max(25, ceil(文字宽度)) × 25` |
| 变量读取 | `VariableBlock` | `FloatBlock` | `100` | `QColor(194,92,0)` | `(文字宽度 + 12) × 25` |
| 自定义参数读取 | `CustomParamBlock` | `FloatBlock` | `105` | `QColor(82,45,122)` | `(文字宽度 + 12) × 25` |
| 当前 x 坐标 | `RobotCoordBlock` | `FloatBlock` | `102` | `QColor(42,86,150)` | `(文字宽度 + 12) × 25` |
| 当前 y 坐标 | `RobotCoordBlock` | `FloatBlock` | `102` | `QColor(42,86,150)` | `(文字宽度 + 12) × 25` |
| 前方类型 | `RobotFrontMapBlock` | `FloatBlock` | `103` | `QColor(42,86,150)` | `(文字宽度 + 12) × 25` |
| 读取列表某项 | `ListGetBlock` | `FloatBlock` | `101` | `QColor(125,28,38)` | 由文字、列表名框和下标数值块共同决定，高度至少 `25` |
| 列表长度 | `ListSizeBlock` | `FloatBlock` | `104` | `QColor(125,28,38)` | 由文字和列表名框共同决定，高度至少 `25` |

## 运算积木

| 积木 | 类 | 父类 | 颜色 | 初始尺寸 |
| --- | --- | --- | --- | --- |
| 一元运算 | `UnaryOpBlock` | `FloatBlock` | `QColor(42,105,86)` | `(左内边距 + 运算符文字宽度 + 中间距 + 参数长度 + 右内边距) × (max(参数高度, 25) + 上下内边距)` |
| 二元运算 | `BinaryOpBlock` | `FloatBlock` | `QColor(42,105,86)` | `(左参数、运算符、右参数及内边距总和) × (max(左参数高度, 右参数高度, 25) + 上下内边距)` |

当前工具箱中的一元运算：

| 显示文本 | 说明 |
| --- | --- |
| `sin` | 正弦 |
| `cos` | 余弦 |
| `tan` | 正切 |
| `asin` | 反正弦 |
| `acos` | 反余弦 |
| `atan` | 反正切 |
| `ln` | 自然对数 |
| `log10` | 常用对数 |
| `floor` | 向下取整 |
| `abs` | 绝对值 |
| `not` | 逻辑非 |

当前工具箱中的二元运算：

| 显示文本 | 说明 |
| --- | --- |
| `+` | 加法 |
| `-` | 减法 |
| `*` | 乘法 |
| `/` | 除法 |
| `pow` | 幂 |
| `arg` | 角度 / 参数运算 |
| `max` | 最大值 |
| `min` | 最小值 |
| `==` | 相等 |
| `!=` | 不相等 |
| `<` | 小于 |
| `>` | 大于 |
| `and` | 逻辑与 |
| `or` | 逻辑或 |

## 颜色函数

| 函数 | 颜色 | 使用位置 |
| --- | --- | --- |
| `variableColor()` | `QColor(194,92,0)` | 变量读取、变量赋值、创建变量按钮 |
| `listColor()` | `QColor(125,28,38)` | 列表读取、列表长度、列表操作、创建列表按钮 |
| `robotCoordColor()` | `QColor(42,86,150)` | 当前坐标、前方类型 |
| `customBlockColor()` | `QColor(82,45,122)` | 自定义帽子、自定义调用、自定义参数、创建自定义积木按钮 |
| `fileButtonColor()` | `QColor(126,70,180)` | 保存、打开、退出按钮 |
