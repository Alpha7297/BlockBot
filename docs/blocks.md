# 积木整理

## 显示规则

| 条件 | 当前规则 |
| --- | --- |
| 沙盒模式 | 显示全部工具箱能力 |
| 第 1 关 | 只显示开始、结束、左转、右转、向前移动 |
| 第 2 关 | 增加等待 |
| 第 3 关 | 增加如果、当、not、==、<、>、当前坐标、前方能否通行 |
| 第 4 关 | 增加变量、赋值、增加变量、floor、abs、算术/逻辑二元运算 |
| 第 5 关 | 增加列表和自定义积木 |
| 第 6 关及之后 | 增加输出 |

## 语句积木

| 显示文本 | 类 | type | 颜色/来源 | 运行行为 |
| --- | --- | --- | --- | --- |
| 开始运行 | `StartBlock` | `-1` | `QColor(156,118,42)` | 程序入口，不直接执行动作 |
| 结束运行 | `EndBlock` | `-4` | `QColor(156,118,42)` | 标记程序结束；运行前要求开始链中存在结束块 |
| 左转 | `CodeBlock` | `0` | `Qt::blue` | `RobotActions::turnLeft()` |
| 右转 | `CodeBlock` | `1` | `Qt::blue` | `RobotActions::turnRight()` |
| 向前移动 | `FloatCodeBlock` | `3` | 继承 `CodeBlock` | 消耗动作帧；每次向前移动一格 |
| 等待 | `FloatCodeBlock` | `4` | 继承 `CodeBlock` | 消耗动作帧；等待指定帧数 |
| 如果 | `ControlCodeBlock` | `5` | `QColor(54,92,122)` | 条件非 0 时进入内部链 |
| 当 | `ControlCodeBlock` | `6` | `QColor(54,92,122)` | 条件非 0 时循环执行内部链 |
| 输出 | `OutputBlock` | `12` | `QColor(130,76,180)` | 调用 `showMessage(messageText,value)` |
| 将变量设为 | `SetVariableBlock` | `7` | `variableColor()` | 设置变量值 |
| 将变量增加 | `IncreaseVariableBlock` | `14` | `variableColor()` | 在当前变量值上增加 |
| 在列表末尾加入 | `PushListBlock` | `8` | `listColor()` | 向列表尾部追加值 |
| 设置列表某项 | `SetListBlock` | `9` | `listColor()` | 设置指定下标的列表值 |
| 删除列表某项 | `RemoveListItemBlock` | `13` | `listColor()` | 删除指定下标的列表项 |
| 清空列表 | `ClearListBlock` | `10` | `listColor()` | 清空整个列表 |
| 自定义积木定义 | `CustomHatBlock` | `-3` | `customBlockColor()` | 自定义积木入口 |
| 自定义积木调用 | `CustomCallBlock` | `11` | `customBlockColor()` | 进入对应 `CustomHatBlock` |

## 数值积木

| 显示文本 | 类 | type | 颜色/来源 | 说明 |
| --- | --- | --- | --- | --- |
| 普通数值 | `FloatBlock` | 构造参数决定 | `QColor(92,102,116)` | 可编辑常量 |
| 变量读取 | `VariableBlock` | `100` | `variableColor()` | 从 `runtimeState.variables()` 生成 |
| 自定义参数读取 | `CustomParamBlock` | `105` | `customBlockColor()` | 读取当前自定义调用栈参数 |
| 当前 x 坐标 | `RobotCoordBlock` | `102` | `robotCoordColor()` | 返回机器人 `gridx` |
| 当前 y 坐标 | `RobotCoordBlock` | `102` | `robotCoordColor()` | 返回机器人 `gridy` |
| 前方能否通行 | `RobotFrontMapBlock` | `103` | `robotCoordColor()` | 前方可通行为 1，否则为 0；激活地刺视为不可通行 |
| 读取列表某项 | `ListGetBlock` | `101` | `listColor()` | 读取指定下标列表值 |
| 列表长度 | `ListSizeBlock` | `104` | `listColor()` | 返回列表长度 |

## 运算积木

### 一元运算

| 显示文本 | type | 说明 |
| --- | --- | --- |
| `floor` | `8` | 向下取整 |
| `abs` | `9` | 绝对值 |
| `not` | `10` | 逻辑非 |

### 二元运算

| 显示文本 | type | 说明 |
| --- | --- | --- |
| `+` | `0` | 加法 |
| `-` | `1` | 减法 |
| `*` | `2` | 乘法 |
| `/` | `3` | 除法 |
| `pow` | `4` | 幂 |
| `max` | `6` | 最大值 |
| `min` | `7` | 最小值 |
| `==` | `8` | 相等 |
| `!=` | `9` | 不相等 |
| `<` | `10` | 小于 |
| `>` | `11` | 大于 |
| `and` | `12` | 逻辑与 |
| `or` | `13` | 逻辑或 |

## 颜色函数

| 函数 | 当前值 | 使用位置 |
| --- | --- | --- |
| `variableColor()` | `QColor(194,92,0)` | 变量读取、变量赋值、创建变量按钮 |
| `listColor()` | `QColor(125,28,38)` | 列表读取、列表长度、列表操作、创建列表按钮 |
| `robotCoordColor()` | `QColor(42,86,150)` | 当前坐标、前方能否通行 |
| `customBlockColor()` | `QColor(82,45,122)` | 自定义定义、调用、参数、创建自定义按钮 |
| `fileButtonColor()` | `QColor(92,98,108)` | 设置、保存、打开、退出、信息、全屏按钮 |
