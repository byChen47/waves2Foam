# stokesI-HOS 的 HOS-NWT 造波子算例

## 这个算例做什么

为 `stokesI-HOS` 大型水池算例生成规则波 SWENSE 数据
（`Results/modes_HOS_SWENSE.dat`）。

HOS 水池：200 m（长）× 1 m（宽）× 100 m（水深），规则波振幅 0.025 m、
周期 3 s（与原来 StokesI 的 H=0.05 m、T=3 s 等价），造波 60 s。

## 运行步骤

```bash
bash runHOS-NWT.sh                          # 生成 Results/modes_HOS_SWENSE.dat
cp Results/modes_HOS_SWENSE.dat ../constant/  # 交给 CFD 算例
```

## 波浪设置文件 `input_HOS-NWT.dat` 要点

| 关键字 | 取值 | 说明 |
|---|---|---|
| `xlen` | 200.0 | HOS 水池长；只需覆盖 CFD 耦合区（HOS x=20..50） |
| `ylen` | 1.0 | 宽度（n2=1，二维） |
| `h` | 100.0 | 水深，与 CFD 一致 |
| `icase` | 2 | 规则波 |
| `amp_mono` | 0.025 | 振幅（m） |
| `nu_mono` | 0.3333333333 | 频率（Hz），T=3 s |
| `igeom` | 1 | 活塞式造波机 |
| `Tramp` | 3.0 | 斜坡时长（s） |
| `T_stop` | 60.0 | 总时长（s），覆盖 CFD 30 s |
| `f_out` | 20 | 输出频率（Hz） |
| `i_sw` | 1 | **SWENSE 输出，必须 =1** |

完整参数含义见 `HOSwaves2Foam/tutorials/HOS-NWT/README.md`。
