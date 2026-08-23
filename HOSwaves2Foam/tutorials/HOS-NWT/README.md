# HOS-NWT 规则波造波算例

## 这个算例做什么

用 **HOS-NWT**（高阶谱数值波浪水池）生成一份**规则波**的 SWENSE 数据文件
`Results/modes_HOS_SWENSE.dat`，供 `waveFoam-HOS` 算例通过 Grid2Grid 耦合
使用。HOS-NWT 负责“造波”，它本身不做 CFD。

水池参数：20 m（长）× 1 m（宽）× 0.4 m（水深），规则波振幅 0.05 m、
周期 2 s（波高 0.1 m）。

## 运行流程（从哪一步开始）

本算例是两步流程中的**第 3 步：HOS 造波**——只有修改波浪参数时才需要
运行；日常跑 CFD 直接去 `waveFoam-HOS` 执行第 4 步。完整流程和首次
使用的编译步骤见 [tutorials/README.md](../README.md)。

```text
第 3 步（本算例）：HOS-NWT 造波 → Results/modes_HOS_SWENSE.dat
第 4 步：waveFoam-HOS 跑 CFD（bash ./Allrun）
```

`waveFoam-HOS/constant/` 已内置同一份数据，日常直接做第 4 步即可。

## 目录结构

```text
HOS-NWT/
├── input_HOS-NWT.dat    # HOS-NWT 输入文件（波浪就靠它设置）
├── prob.inp             # 测点文件（本算例未启用）
└── runHOS-NWT.sh        # 一键运行脚本
```

## 运行步骤（第 3 步：造波）

前置：HOS-NWT 已编译（`HOSwaves2Foam/scripts/buildHOS.sh`，产物
`src/HOS-NWT/build/HOS_NWT`）；环境已加载（见 tutorials/README.md）。

```bash
# 1. 造波（在 HOS-NWT 目录内；自动找 build/HOS_NWT，也可显式传路径）
bash runHOS-NWT.sh

# 2. 把数据交给 CFD 算例
cp Results/modes_HOS_SWENSE.dat ../waveFoam-HOS/constant/

# 3. 跑 CFD（第 4 步）
cd ../waveFoam-HOS && bash ./Allrun
```

验证：`Results/modes_HOS_SWENSE.dat` 生成、日志无 NaN；CFD 的
`log.setWaveField` 出现 `GRID2GRID Vol2Vol is initialized`。

## 波浪设置文件 `input_HOS-NWT.dat` 详解

格式是“说明文字 :: 关键字 :: 数值”，全部使用**物理单位（m、s、Hz）**。
下面只解释与波浪有关的项：

| 关键字 | 含义 | 本算例 | 想改波浪时 |
|---|---|---|---|
| `xlen` | 水槽长度（m） | 20.0 | 与 CFD 松弛区位置匹配 |
| `ylen` | 水槽宽度（m） | 1.0 | 2D（n2=1）时随意，>0 即可 |
| `h` | 水深（m） | 0.4 | **必须与 CFD 网格水深一致** |
| `icase` | 算例类型：2=规则波 | 2 | 规则波保持 2 |
| `amp_mono` | 规则波**振幅**（m） | 0.05 | 波高 = 2×振幅，如 H=0.2 → 写 0.1 |
| `nu_mono` | 频率（Hz） | 0.5 | 周期 T = 1/nu，如 T=2.5 s → 写 0.4 |
| `theta_mono` | 传播方向与 x 轴夹角（°） | 0 | 斜浪时改这里 |
| `ph_mono` | 初始相位（rad） | 0 | 一般不动 |
| `i_wmk` | 造波机非线性阶数 | 1 | 1=线性造波机 |
| `igeom` | 造波机类型：1=活塞式，2=铰接式 | 1 | 浅水用活塞式更稳定 |
| `Tramp` | 造波斜坡时长（s） | 2.0 | 越大启动越缓 |
| `T_stop` | 模拟总时长（**物理秒**） | 12.0 | 必须覆盖“波传到耦合区 + CFD 全程” |
| `f_out` | 输出频率（Hz） | 20 | 数据时间分辨率，每周期 ≥ 10~20 点 |
| `i_sw` | **SWENSE 输出开关** | 1 | **必须 =1**，否则 Grid2Grid 读不了 |
| `imodes` | Tecplot 谱模态输出 | 0 | 那个文件不是 Grid2Grid 用的 |

其余项（`ibat`、`xd_mono`、`iabsnb`、`xabsf`、`toler`、`idim` 等）保持默认
即可，含义见 `HOSwaves2Foam/doc/使用说明.md` 第 4 节。

## 常见问题

* 报 `Cannot open file 'Results/vol_energy.dat'`：当前目录缺少 `Results/`，
  用 `runHOS-NWT.sh` 运行会自动创建。
* 输出 NaN：波浪参数过陡或造波机类型不合适，试试减小振幅或用 `igeom 1`。
