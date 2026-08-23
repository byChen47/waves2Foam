# stokesI-HOS-3D 的三维 HOS-NWT 造波子算例

## 这个算例做什么

为 `stokesI-HOS-3D` 生成**三维斜向规则波**的 SWENSE 数据
（`Results/modes_HOS_SWENSE.dat`）。

HOS 水池：200 m（长）× 20 m（宽）× 100 m（水深），振幅 0.025 m、
周期 3 s、波向与 x 轴成 15°（斜波 → 三维），造波 30 s。
**需要 n2=8（y 向模态）的 3D 版 HOS-NWT**（见下）。

## 运行步骤

```bash
# 先编译 3D 版 HOS-NWT（n1=128, n2=8）
bash HOSwaves2Foam/scripts/buildHOS3D.sh

# 再运行造波（自动找 src/HOS-NWT/build3D/HOS_NWT）
bash runHOS-NWT.sh
cp Results/modes_HOS_SWENSE.dat ../constant/
```

## 波浪设置文件 `input_HOS-NWT.dat` 要点

| 关键字 | 取值 | 说明 |
|---|---|---|
| `xlen` / `ylen` / `h` | 200 / 20 / 100 | 水池长 / 宽 / 水深 |
| `icase` | 2 | 规则波 |
| `amp_mono` | 0.025 | 振幅（m） |
| `nu_mono` | 0.3333333333 | 频率（Hz），T=3 s |
| `theta_mono` | 15.0 | **波向角（°）**，>0 才有 y 向分量 |
| `igeom` | 1 | 活塞式造波机 |
| `Tramp` | 3.0 | 斜坡时长（s） |
| `T_stop` | 30.0 | 总时长（s），覆盖 CFD 15 s |
| `f_out` | 20 | 输出频率（Hz） |
| `i_sw` | 1 | **SWENSE 输出，必须 =1** |

## 为什么需要 `buildHOS3D.sh`

2D 版 HOS-NWT 的 y 向模态数是编译期常量 `n2 = 1`。三维必须 `n2 > 1`，
所以提供了专用构建脚本：临时把 `common_vars.f90` 改成
`n1 = 128`、`n2 = 8`，编译到 `build3D/` 后自动还原源码（2D 教程的
`build/HOS_NWT` 不受影响）。n1 用 128 是为了控制 SWENSE 文件大小
（三维文件 ≈ n1×n2×8 记录/步）。

验证数据质量：`Results/probes.dat` 中三个测点
（(25,5)、(35,10)、(45,15)）的波高应在 ±0.05 m 内振荡；若出现
随时间线性增长的异常值，说明 HOS 运行本身有问题。
