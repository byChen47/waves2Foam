# stokesI-HOS-3D：三维 HOS 造波大型水池算例

## 这个算例做什么

在 `stokesI-HOS`（624 m 长、100 m 水深）的基础上增加**宽度方向**，做成
**三维**数值波浪水池，波浪由 **HOS-NWT（n1=128、n2=8、15° 斜向规则波）**
生成，经 Grid2Grid 三维重构注入 waveFoam：

* CFD 域：624 m（x）× 20 m（y，y=0.5..19.5 m）× 200 m（z），网格
  156×10×33 ≈ 51,000 单元；
* HOS 水池：200 m（x）× 20 m（y）× 100 m（水深），振幅 0.025 m、
  T = 3 s、传播方向与 x 轴成 15°；
* 入口松弛区 x = -312..-282 m（对应 HOS x = 20..50 m），出口松弛区
  x = 282..312 m，模拟 15 s。

> 与 2D 版相同的说明：基础网格竖向 6 m 单元**无法分辨 0.05 m 的波面**，
> 生产计算请打开 `Allrun` 里的 `topoSet`/`refineMesh` 加密，并配合并行。

## 运行顺序（两步，先 HOS 后 OpenFOAM）

```text
第 1 步：HOS-NWT 生成波浪数据（只有改波浪参数时才需要重跑）
          → Results/modes_HOS_SWENSE.dat → 拷到 constant/
第 2 步：运行 OpenFOAM 算例（Allrun）
          → blockMesh → setWaveField → waveGaugesNProbes → waveFoam
```

**算例里已经内置了三维 SWENSE 数据**（`constant/modes_HOS_SWENSE.dat`，
89 MB），所以直接跑第 2 步即可；第 1 步用于自己换波参数。

## 运行步骤

### 前置（只需一次）

```bash
# 三维 HOS-NWT 需要 n2 > 1（y 向模态），用专用脚本编译（默认 n1=128, n2=8）
bash HOSwaves2Foam/scripts/buildHOS3D.sh
# 产物：HOSwaves2Foam/src/HOS-NWT/build3D/HOS_NWT（2D 版 build/ 不受影响）
```

以及已编译的 `libGrid2Grid.so`、`libwaves2FoamGrid2Grid.so`。

### 第 1 步：重新生成三维波浪数据（可选）

```bash
cd HOSwaves2Foam/tutorials/stokesI-HOS-3D/HOS-NWT
bash runHOS-NWT.sh                     # 自动找 build3D 的可执行文件
cp Results/modes_HOS_SWENSE.dat ../constant/
```

### 第 2 步：运行 CFD 算例

```bash
cd HOSwaves2Foam/tutorials/stokesI-HOS-3D
bash ./Allrun          # restore0Dir → blockMesh → setWaveField →
                       # waveGaugesNProbes → waveFoam（15 s）
```

验证点：

* `log.setWaveField` 出现 `GRID2GRID Vol2Vol is initialized`；
* `postProcessing/surfaceElevationAnyName/0/surfaceElevation.dat` 是沿
  y=10 m 中线的波高时序（waveGaugesNProbes 输出）；
* 运行 `postG2G constant/Grid2Grid.dict` 可得到 Grid2Grid 在多个
  (x, y) 点的重构波高 `waveElevation.dat`（三维探针）。

## 三维波浪设置要点

### HOS 侧（`HOS-NWT/input_HOS-NWT.dat`）

| 参数 | 取值 | 说明 |
|---|---|---|
| `ylen` | 20.0 | 水池宽度（m），决定 y 向模态波长 |
| `theta_mono` | 15.0 | 传播方向与 x 轴夹角（°），>0 才是斜向/三维波 |
| `amp_mono` / `nu_mono` | 0.025 / 1/3 | 振幅 / 频率（T=3 s） |
| `T_stop` / `f_out` | 30 / 20 | 数据时长 / 输出频率 |
| `i_sw` | 1 | SWENSE 输出（Grid2Grid 必需） |

### 编译参数（`scripts/buildHOS3D.sh`）

三维需要 `n2 > 1`（y 向模态数，编译期常量）。脚本会临时把
`src/HOS-NWT/sources/variabledef/common_vars.f90` 的
`n1 = 513`、`n2 = 1` 改成 `n1 = 128`、`n2 = 8`，编译到 `build3D/`
后**自动还原源码**（2D 教程不受影响）。n1 调小是为了控制 SWENSE 文件
大小（文件大小 ∝ n1×n2×时间）。

### CFD 侧

* `constant/waveProperties`：`Grid2GridCoeffs.referencePosition (332 0 0)`
  （OF x=-312 → HOS x=20，y 无偏移，CFD y=0.5..19.5 落在 HOS y=0..20 内部）；
* `constant/Grid2Grid.dict`：
  * `addedMode false;` —— **三维必须关**。Grid2Grid 对 n2>1 时的附加模态
    重构有缺陷（会产生随时间线性增长的虚假速度），关闭后正常；
  * `cutAtElevation true;` 与 `zMax 1.0`：只重构到波面附近，避免空气侧
    指数放大；
  * 探针用三维坐标 `(x y z)`（波高 `(x y)`），见文件内 `waveProbe` 块。
* `system/blockMeshDict`：y = 0.5..19.5、`ynumber 10`；`sides` 改为
  `wall`（不再是 2D 的 empty）。

## 常见问题

* `waveFoam` 第一步 Courant 爆炸（~1e11）：多半是 `addedMode` 没关，
  检查 `Grid2Grid.dict`；
* 波面看不出起伏：竖向 6 m 单元无法分辨小波，属预期，需网格加密；
* 三维数据文件很大（约 89 MB/30 s）：想更小就降低 `buildHOS3D.sh` 的
  n1/n2 或 `T_stop`。

## 目录结构

```text
stokesI-HOS-3D/
├── 0.orig/                  # 初始场模板（sides 为壁面）
├── HOS-NWT/                 # 三维造波子算例（n1=128, n2=8, 斜向波）
├── constant/
│   ├── waveProperties       # Grid2Grid 耦合配置
│   ├── Grid2Grid.dict       # 数据桥配置（addedMode false）
│   ├── modes_HOS_SWENSE.dat # 三维波浪数据（已内置）
│   └── probeDefinitions     # waveGaugesNProbes 测点（y=10 中线）
├── system/                  # blockMesh/controlDict/加密与并行配置
└── Allrun
```
