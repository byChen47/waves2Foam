# HOS-NWT 不规则波造波算例

## 这个算例做什么

用 **HOS-NWT** 生成**不规则波**（JONSWAP 谱）的 SWENSE 数据
`Results/modes_HOS_SWENSE.dat`，供 `waveFoam-HOS-Irregular` 算例使用。

水池与 `HOS-NWT` 算例相同（20 m × 1 m × 0.4 m），波浪参数：
有义波高 Hs = 0.05 m、谱峰周期 Tp = 1 s、峰升因子 γ = 3.3。

## 目录结构

```text
HOS-NWT-Irregular/
├── input_HOS-NWT.dat    # ★ 波浪输入（不规则波参数在这里）
├── prob.inp             # 测点文件（未启用）
└── runHOS-NWT.sh        # 一键运行脚本
```

## 运行步骤

```bash
bash runHOS-NWT.sh                          # 运行 HOS-NWT
cp Results/modes_HOS_SWENSE.dat ../waveFoam-HOS-Irregular/constant/
cd ../waveFoam-HOS-Irregular && bash ./Allrun
```

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
