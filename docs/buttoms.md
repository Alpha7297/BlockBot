# 按钮整理

本文档记录 `ui/App.cpp` 和 `ui/Widgets.h` 中当前按钮、滑条和主要 UI 控件的类型、颜色、宽度和长度。

代码中按钮通常使用屏幕坐标描述矩形，本文档统一写作 `宽 × 高`。

## 按钮基础类

| 类型 | 类 | 父类 | 颜色 | 尺寸 |
| --- | --- | --- | --- | --- |
| 图形按钮默认形态 | `Button` | `QGraphicsPolygonItem` | `QColor(80,140,235)` | 近似圆形，直径 `20` |
| 图形按钮阴影 | `Button::shadow` | `QGraphicsPolygonItem` | `QColor(180,180,180,120)` | `24 × 24` |
| 播放图标 | `Button::setPlayShape()` | `Button` | `QColor(44,135,82)` | 约 `18 × 20` |
| 闪电图标 | `Button::setLightningShape()` | `Button` | `QColor(202,150,36)` | 约 `15 × 22` |
| 停止图标 | `Button::setStopShape()` | `Button` | `QColor(180,48,48)` | 约 `22 × 22` |
| 文本按钮默认形态 | `TextButton` | `QGraphicsPolygonItem` | `QColor(80,120,170)` | 默认 `(文字宽度 + 20) × (文字高度 + 10)` |
| 右键菜单按钮 | `ContextMenuButton` | `Button` | `QColor(70,80,96)` | `56 × 24` |
| 滚动条滑块 | `ScrollSlider` | `QGraphicsPolygonItem` | `QColor(75,85,99)` | `18 × 60` |

## 当前页面按钮

| 按钮 | 类 | 颜色 | 宽 × 高 | 位置 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 运行 | `TextButton` | `QColor(44,135,82)` | `150 × 40` | `(15,410)` | 位于舞台下方 |
| 停止 | `TextButton` | `QColor(180,48,48)` | `150 × 40` | `(175,410)` | 位于舞台下方 |
| 保存 | `TextButton` | `QColor(126,70,180)` | `60 × 60` | `(1060,10)` | 顶部文件按钮 |
| 打开 | `TextButton` | `QColor(126,70,180)` | `60 × 60` | `(1130,10)` | 顶部文件按钮 |
| 创建新变量 | `TextButton` | `QColor(194,92,0)` | `160 × 40` | `(10,530)` | 左侧工具按钮 |
| 创建新列表 | `TextButton` | `QColor(125,28,38)` | `160 × 40` | `(10,580)` | 左侧工具按钮 |
| 创建自定义积木 | `TextButton` | `QColor(82,45,122)` | `160 × 40` | `(10,630)` | 左侧工具按钮 |
| 退出 | `TextButton` | `QColor(126,70,180)` | `160 × 40` | `(10,680)` | 左侧底部按钮 |

## 当前非按钮控件

| 控件 | 类 | 颜色 | 宽 × 高 | 位置 | 说明 |
| --- | --- | --- | --- | --- | --- |
| LOGO 占位框 | `QGraphicsRectItem` | `QColor(38,44,52)` | `1040 × 60` | `(10,10)` | 当前用长方形和 `LOGO1.png` 临时代替 |
| LOGO 边框 | `QPen` | `QColor(90,100,112)` | 线宽 `1.5` | 同 LOGO | LOGO 占位框描边 |
| 信息框 | `QGraphicsRectItem` | `QColor(230,233,238)` | `320 × 60` | `(10,460)` | 舞台下方预留功能框 |
| 工具箱滑条 | `ScrollSlider` | `QColor(75,85,99)` | `18 × 60` | `toolbox` 右侧 | 根据工具箱内容滚动 |
| 工作区滑条 | `ScrollSlider` | `QColor(75,85,99)` | `18 × 60` | `workspace` 右侧 | 根据工作区内容滚动 |

## 已停用按钮

| 名称 | 当前状态 | 说明 |
| --- | --- | --- |
| 快速运行 | 不再创建 | 新布局中已经删除 |
| 旧 `runButton` 图形按钮 | 指针保留但设为 `nullptr` | 当前使用文本按钮 `运行` |
| 旧 `fastRunButton` 图形按钮 | 指针保留但设为 `nullptr` | 当前没有对应 UI |
| `testButton` | 指针保留但设为 `nullptr` | 当前测试入口由其他逻辑处理 |
