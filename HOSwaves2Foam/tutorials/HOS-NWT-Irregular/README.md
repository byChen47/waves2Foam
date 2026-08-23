# HOS-NWT 不规则波造波算例

## 这个算例做什么

用 **HOS-NWT** 生成**不规则波**（JONSWAP 谱）的 SWENSE 数据
`Results/modes_HOS_SWENSE.dat`，供 `waveFoam-HOS-Irregular` 算例使用。

水池与 `HOS-NWT` 算例相同（20 m × 1 m × 0.4 m），波浪参数：
有义波高 Hs = 0.05 m、谱峰周期 Tp = 1 s、峰升因子 γ = 3.3。

## 运行流程（从哪一步开始）

本算例是两步流程中的**第 3 步：HOS 造波**（不规则波）——只有修改谱参数
时才需要运行；日常跑 CFD 直接去 `waveFoam-HOS-Irregular` 执行第 4 步。
完整流程见 [tutorials/README.md](../README.md)。

```text
第 3 步（本算例）：HOS-NWT 造不规则波 → Results/modes_HOS_SWENSE.dat
第 4 步：waveFoam-HOS-Irregular 跑 CFD（bash ./Allrun）
```

## 目录结构

```text
HOS-NWT-Irregular/
├── input_HOS-NWT.dat    # ★ 波浪输入（不规则波参数在这里）
├── prob.inp             # 测点文件（未启用）
└── runHOS-NWT.sh        # 一键运行脚本
```

## 运行步骤（第 3 步：造波）

前置：`buildHOS.sh` 已编译 HOS-NWT；环境已加载（见 tutorials/README.md）。

```bash
# 1. 造波（在 HOS-NWT-Irregular 目录内）
bash runHOS-NWT.sh

# 2. 把数据交给 CFD 算例
cp Results/modes_HOS_SWENSE.dat ../waveFoam-HOS-Irregular/constant/

# 3. 跑 CFD（第 4 步）
cd ../waveFoam-HOS-Irregular && bash ./Allrun
```

验证：`Results/modes_HOS_SWENSE.dat` 生成、无 NaN；CFD 的
`log.setWaveField` 出现 `GRID2GRID Vol2Vol is initialized`。

## 波浪设置文件 `input_HOS-NWT.dat` 详解

与规则波算例的区别在 `icase=4` 和下面的谱参数：

| 关键字 | 含义 | 本算例 | 想改波浪时 |
|---|---|---|---|
| `icase` | 算例类型：4=不规则波 | 4 | 不规则波保持 4 |
| `Hs` | 有义波高（m） | 0.05 | 直接改，如 0.08 |
| `Tp` | 谱峰周期（s） | 1.0 | 直接改 |
| `gamma` | JONSWAP 峰升因子 | 3.3 | 3.3 是标准值 |
| `iseed` | 随机种子 | 513 | 换种子得到不同相位组合 |
| `T_stop` | 总时长（物理秒） | 40.0 | 不规则波需要几十个谱峰周期才有统计意义 |
| `f_out` | 输出频率（Hz） | 20 | 保证短波分量每周期 ≥ 10 点 |
| `i_sw` | SWENSE 输出开关 | 1 | **必须 =1** |

其余（水池尺寸 `xlen/ylen/h`、造波机 `i_wmk/igeom/Tramp`、消波
`iabsnb/xabsf`）与 `HOS-NWT/README.md` 相同。

注意：换 `Hs/Tp` 后，CFD 侧的 `endTime` 和 HOS 的 `T_stop` 都要相应调整；
波浪统计意义上需要足够长的模拟时间。
