# 按钮整理

## 基础控件

| 控件 | 类 | 当前来源 | 说明 |
| --- | --- | --- | --- |
| 图形按钮 | `Button` | `ui/Widgets.h` | 默认圆形，直径约 20，默认色 `QColor(80,140,235)` |
| 图形按钮阴影 | `Button::shadow` | `ui/Widgets.h` | `24*24`，`QColor(180,180,180,120)` |
| 文本/贴图按钮 | `TextButton` | `ui/App.cpp` | `QGraphicsPolygonItem`，默认色 `QColor(80,120,170)`，多数实际按钮会改色并贴图 |
| 右键菜单按钮 | `ContextMenuButton` | `ui/App.cpp` | `56*24`，`QColor(70,80,96)` |
| 滚动条滑块 | `ScrollSlider` | `ui/Widgets.h` | `18*60`，默认色 `QColor(75,85,99)`，当前使用 `slider.png` 贴图 |
| 关卡/沙盒提示面板 | `LevelHintPanel` | `ui/hint.cpp` | `800*466`，点击面板隐藏 |

## 固定布局常量

| 名称 | 当前值 |
| --- | --- |
| 窗口大小 | `1200*800` |
| 舞台 | `(10,80)`，`320*320` |
| 工具箱 | `(340,80)`，`280*710` |
| 工作区 | `(630,80)`，`560*710` |
| 顶部按钮层级 | `topUiZ=100020` |

## 当前页面按钮

| 按钮 | 类 | 颜色 | 尺寸 | 位置 | 贴图 | 显示条件/说明 |
| --- | --- | --- | --- | --- | --- | --- |
| 运行 | `TextButton` | `QColor(44,135,82)` | `150*scaledAssetHeight("run.png",150,40)` | `(15,410)` | `run.png` | 始终显示；运行中显示“运行中” |
| 停止 | `TextButton` | `QColor(180,48,48)` | `150*scaledAssetHeight("stop.png",150,40)` | `(175,410)` | `stop.png` | 始终显示 |
| 全屏舞台 | `TextButton` | `fileButtonColor()` | `60*60` | `(260,577)` | `larger.png` | 文字隐藏；展开舞台时隐藏 |
| 缩小舞台 | `TextButton` | `fileButtonColor()` | `60*60` | 动态：`informationRect().right()+20,informationRect().top()` | `smaller.png` | 默认隐藏；展开舞台时显示 |
| 创建新变量 | `TextButton` | `variableColor()` | `160*scaledAssetHeight("create_variable.png",160,40)` | `(10,577)` | `create_variable.png` | 沙盒或第 4 关及以后显示 |
| 创建新列表 | `TextButton` | `listColor()` | `160*scaledAssetHeight("create_list.png",160,40)` | `(10,617)` | `create_list.png` | 沙盒或第 5 关及以后显示 |
| 创建自定义积木 | `TextButton` | `customBlockColor()` | `160*scaledAssetHeight("create_custom.png",160,40)` | `(10,657)` | `create_custom.png` | 沙盒或第 5 关及以后显示 |
| 设置 | `TextButton` | `fileButtonColor()` | `60*60` | `(990,10)` | `icons/settings.png` | 文字隐藏 |
| 保存 | `TextButton` | `fileButtonColor()` | `60*60` | `(1060,10)` | `save.png` | 文字隐藏；保存 `.bbot` |
| 打开 | `TextButton` | `fileButtonColor()` | `60*60` | `(1130,10)` | `open.png` | 文字隐藏；支持 `.bbot`，JSON 会弹不安全警告 |
| 退出 | `TextButton` | `fileButtonColor()` | `60*60` | `(10,10)` | `icons/return.png` | 文字隐藏 |
| 信息 | `TextButton` | `fileButtonColor()` | `60*60` | `(80,10)` | `icons/information.png` | 文字隐藏；普通关卡显示关卡提示，沙盒显示地图编辑提示 |

## 非按钮主控件

| 控件 | 类 | 位置/尺寸 | 说明 |
| --- | --- | --- | --- |
| 舞台背景 | `QGraphicsRectItem` | 当前舞台位置和大小 | 白色底，按当前地图尺寸绘制格子 |
| 地图格子 | `SandboxMapCellItem` | 舞台内网格 | 沙盒模式下左键点击可选择 `empty floor`、`spikeup`、`wall` |
| 机器人 | `Robot` | 舞台格子坐标 | `QGraphicsPixmapItem`，按方向旋转/绘制 |
| 信息面板图片 | `QGraphicsPixmapItem` | `(10,460)`，宽 320 | 使用 `information.png`；失败时用 fallback 矩形 |
| 步数文字 | `QGraphicsTextItem` | `(82,494)` | 白色 Arial Bold 20 |
| 时间文字 | `QGraphicsTextItem` | `(235,494)` | 白色 Arial Bold 20 |
| 工具箱滑块 | `ScrollSlider` | `toolbox` 右侧 | 贴图 `slider.png` |
| 工作区滑块 | `ScrollSlider` | `workspace` 右侧 | 贴图 `slider.png` |

## 已停用/保留指针

| 名称 | 当前状态 |
| --- | --- |
| `runButton` | 指针保留，但当前界面不创建旧图形按钮 |
| `fastRunButton` | 指针保留，但当前界面不创建 |
| `testButton` | 指针保留并设为 `nullptr`，当前运行入口由“运行”按钮和关卡测试流程处理 |
