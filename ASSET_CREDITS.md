# 项目素材记录

`assets/ui/supply-atlas.png` 为内置图像生成工具制作的 4 × 4 商店图片图集，包含七种武器、六种防御塔、弹药箱、医疗箱和核心。提示词保存在 `assets/ui/ART_PROMPT.txt`。图集按格子独立显示在武器、防御塔卡片与详情区，属于预生成 AI 辅助美术。

`assets/icons/ember-warden.png` 为内置图像生成工具制作的守望者游戏图标，属于预生成的 AI 辅助美术。Windows 图标 `ember-warden.ico` 由该图导出六种尺寸，已用于可执行文件和窗口。设计提示词保存在 `assets/icons/DESIGN.md`。

`assets/pixel/citadel-atlas.png` 为本项目通过内置图像生成工具制作的 4 × 4 像素材质图集，生成于 2026-11-19。统一描述为冷青石砌遗迹、苔藓、古铜金属和暖金火光。16 种材质覆盖地面、道路、石墙、木材、金属、符文、怪物装甲、树叶、瓦片、水晶、熔岩、雕刻石、骨骼和水面。原始生成图直接纳入项目，着色器对地面采用 16 × 16、其他材质采用 32 × 32 的像素簇采样。新版默认降低纹理对比，远处细节随距离衰减，并使用纹理层级过滤减少闪烁。

生成方式为内置 image_gen。完整提示词保存在 `assets/pixel/ART_PROMPT.txt`。它属于预生成的 AI 辅助美术。

角色、武器、防御塔、建筑与粒子由项目代码组合几何体绘制。声音由 `src/Sound.cpp` 在运行时进行程序合成。项目依赖的 GLFW、GLAD、GLM、Dear ImGui、stb 保留各自许可证，测试构建另使用 Catch2。


中文与日文界面使用 Noto Sans CJK SC Regular，字体文件为 `assets/fonts/NotoSansCJKsc-Regular.otf`，保持官方原文件。许可证为 SIL Open Font License 1.1，完整文本保存在 `licenses/NotoSansCJK-OFL.txt`。

[字体官方仓库](https://github.com/notofonts/noto-cjk) · [官方字体许可](https://github.com/notofonts/noto-cjk/blob/main/Sans/LICENSE)
