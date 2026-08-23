# HOS / Grid2Grid 参数与使用详解

本文档是 HOS 耦合链路中 **Grid2Grid 数据桥**（`libGrid2Grid.so`）和
waves2Foam 侧耦合参数（`Grid2GridCoeffs`）的完整参考，逐项说明每个
参数的用途、取值范围和相互约束。参数解析以 Grid2Grid 源码为准
（`postGrid2Grid.inc`、`hosNWT.inc`、`hosOcean.inc`、`HOSVol2Vol.inc`）。

## 1. 链路与两个配置文件

```text
HOS-NWT / HOS-ocean 生成 SWENSE mode 文件
        │  constant/Grid2Grid.dict   ← 数据桥怎么读、怎么重构
        ▼
libGrid2Grid.so
        │  constant/waveProperties 的 Grid2GridCoeffs  ← 坐标/时间怎么对齐
        ▼
libwaves2FoamGrid2Grid.so → waves2Foam 松弛区 → waveFoam
```

* `constant/Grid2Grid.dict`：Grid2Grid 自己的配置（HOS 类型、mode 文件、
  重构网格、探针、输出）；
* `constant/waveProperties` 里的 `Grid2GridCoeffs`：waves2Foam 侧的对齐
  参数（时间、坐标、斜坡、字典文件名）。

## 2. `Grid2Grid.dict` 参数详解

### 2.1 顶层

```text
Grid2Grid       HOSNWT;       // 选择 HOS 模型：HOSNWT 或 HOSOcean
writeVTK                false;// true 时把重构的体积场写成 VTK（诊断用）
writeWaveProbe          false;// true 时按 waveProbe 块输出波高时序
writeVelocityProbe      false;// true 时按 velocityProbe 块输出速度时序
writePressureProbe      false;// true 时按 pressureProbe 块输出动压时序
```

* `Grid2Grid`：**必填**。`HOSNWT` = 数值波浪水池数据，`HOSOcean` = 开放
  海域数据。与 mode 文件来源必须一致，否则重构会失败或结果错误。
* 三个 `write*Probe` 默认都是 `false`；诊断时打开，正式 CFD 运行时建议
  关闭（减少 I/O）。

### 2.2 `simulation` 块（后处理/诊断用）

```text
simulation
{
    startTime       0.0;   // 数据起始时间（s）
    endTime		    60.0;  // 数据结束时间（s）
    dt              0.01;  // 参考步长（s）
}
```

用于 `postG2G` 独立后处理工具；CFD 运行时实际时间轴取自 mode 文件头部
（`dtOut`、`Tstop`），这里的取值应与之对应。约束：`startTime ≥ 0`、
`endTime > startTime`、`dt > 0`。

### 2.3 `zMesh` 块（垂向重构网格，HOS 与 vtk 共用）

```text
zMesh
{
    zMin           -100.0;  // 最低 z（m）：通常 = 水深
    zMax            5.0;    // 最高 z（m）：必须 > 0（伸到自由面上方）
    nZMin           33;     // 水面以下 z 节点数
    nZMax           15;     // 水面以上 z 节点数
    zMeshType         uniform; // uniform | sine | meshratio
    zMinRatio       3.0;    // 仅 zMeshType=meshratio：水下最大/最小网格比
    zMaxRatio       3.0;    // 仅 zMeshType=meshratio：水上最大/最小网格比
}
```

* `zMin`/`zMax` 必须覆盖松弛区的水柱和自由面上方空气余量；
  **`zMax ≤ 0` 会直接报错**（`zMax should be positive`）。
* `zMeshType`：
  * `uniform`：等距网格；
  * `sine`：正弦分布网格；
  * `meshratio`：几何加密网格，近自由面最密，`zMinRatio`/`zMaxRatio`
    是最大/最小间距比，必须 > 0。
* `nZMin`/`nZMax` 决定垂向插值节点数：太粗会损失波浪垂向结构，太细则
  增加重构成本。一般水下按水深/波长取 20~40 个。
* 在 HOS 块里用 `$zMesh` 引用该块（文本展开），在 `vtkMesh` 里同样可用。

### 2.4 `HOSNWT` 块（数据源 = HOS-NWT）

```text
HOSNWT
{
    type            NWT;                 // 固定 "NWT"
    filePath        constant/modes_HOS_SWENSE.dat;  // SWENSE mode 文件
    procedure       velocity;            // velocity | potential
    addedMode       true;                // 是否包含附加模态（默认 true）
    fileType        NONE;                // NONE（ASCII）| HDF5（需编译支持）

    extraInterpolationNumber 2;          // 谱模态插值加密倍数（默认 1）
    interpolationOrder  3;               // 1 线性 | 2 二次 | 3 三次（默认 3）
    cutAtElevation      false;           // true 时在波面处截断重构

    isHOSAmpWeight      false;           // 是否启用谱幅值加权
    HOSAmpWeight
    {
        HOSAmpWeightType    polyWeight;  // expWeight | stepWeight | polyWeight
        expWeightCoeffs  { omegaMin 1.0; alpha 2.0; nExpPower 1.0; }
        stepWeightCoeffs { omegaMin 1.0; }
        polyWeightCoeffs { omegaMin 7.0; omegaMax 14.0; }
    }

    $zMesh                                // 展开 2.3 的 zMesh 块
    writeLog        TRUE;                 // 写 Grid2Grid 运行日志
}
```

各参数说明：

| 参数 | 说明 |
|---|---|
| `type` | 必须 `"NWT"`，否则报错 |
| `filePath` | mode 文件路径（相对算例运行目录）。HOS-NWT 用 `i_sw=1` 输出的 `modes_HOS_SWENSE.dat` |
| `procedure` | `velocity`：直接用模态重构速度；`potential`：先重构速度势再求速度。默认 `velocity` |
| `addedMode` | 是否读入/使用附加模态（HOS-NWT 的 `modesadd*` 记录），默认 true |
| `fileType` | `NONE`（ASCII）默认；`HDF5` 需要 Grid2Grid 以 HDF5 支持编译 |
| `extraInterpolationNumber` | 谱模态数在插值前的加密倍数：1=不加密。值越大重构越平滑，成本越高 |
| `interpolationOrder` | 空间/时间样条阶数：1/2/3。越高越光滑（三次样条默认）。它同时决定时间样条窗口 `nSaveT = order+3` |
| `cutAtElevation` | 默认 false；true 时只重构到波面以下（空气侧不重构） |
| `isHOSAmpWeight` + `HOSAmpWeight` | 对谱幅值做频率加权，用于压制数值伪波/改善高频行为。`HOSAmpWeightType` 选 `expWeight`（指数型，参数 `omegaMin/alpha/nExpPower`）、`stepWeight`（阶跃，`omegaMin`）、`polyWeight`（多项式过渡，`omegaMin/omegaMax`）。默认关闭 |
| `$zMesh` | 把 2.3 的 zMesh 内容展开到这里，作为 HOS 重构网格 |
| `writeLog` | true 时打印重构日志 |

### 2.5 `HOSOcean` 块

与 `HOSNWT` 完全相同，仅 `type "Ocean"`，用于 HOS-ocean 的 mode 文件。
两个块可以同时写在字典里，靠顶层 `Grid2Grid HOSNWT;` 或
`Grid2Grid HOSOcean;` 选择用哪个。

### 2.6 `vtkMesh` 块（仅 writeVTK=true 时使用）

```text
vtkMesh
{
    airMesh         true;    // 是否包含空气侧网格（默认 false）
    xMin 0.0;  xMax 18.0;    // 输出范围（m）
    yMin 0.0;  yMax 0.1;
    nX   180;  nY   2;       // 水平节点数
    $zMesh                   // 垂向沿用 zMesh
}
```

参数：`airMesh`（默认 false）、`xMin/xMax/yMin/yMax/zMin/zMax`、
`nX/nY/nZMin/nZMax`、`zMeshType` 与比例。只影响诊断输出，不影响 CFD。

### 2.7 探针块（write*Probe=true 时使用）

```text
waveProbe
{
    outputFile   waveElevation.dat;   // 输出文件名
    probeList
    {
        waveProbes1 { position ( 2.0 0.0 ); }  // (x y)，波高测点
    }
}
velocityProbe
{
    outputFile  velocityProbe.dat;
    probeList
    {
        velocityProbes1 { position ( 2.0 0.0 -0.2 ); }  // (x y z)
    }
}
pressureProbe { ... }  // 同上
```

`position` 是实数数组：波高 2 个分量 `(x y)`，速度/压力 3 个分量
`(x y z)`（单位 m）。

## 3. `waveProperties` 中的 `Grid2GridCoeffs`

```cpp
Grid2GridCoeffs
{
    waveType            Grid2Grid;       // 固定写法
    Tsoft               3;               // 斜坡时间（s）
    referenceTime       0;               // 时间对齐偏移（s）
    referencePosition   ( 332 0 0 );     // 坐标对齐偏移（m）
    referenceTheta      0;               // 绕 z 旋转角（度）
    Grid2GridInputFileName Grid2Grid.dict; // Grid2Grid 字典文件名
}
```

| 参数 | 含义 | 建议 |
|---|---|---|
| `waveType` | 类型标识，必须 `Grid2Grid` | 固定 |
| `Tsoft` | 波信号斜坡时长：前 Tsoft 秒内波幅按正弦从 0 爬到 1，避免启动冲击 | 2~5 s，与 CFD 时间步/周期匹配 |
| `referenceTime` | HOS 时间轴 = OF 时间 + referenceTime | **保持 0**（见 3.2） |
| `referencePosition` | HOS 坐标 = OF 坐标 + referencePosition | 让耦合区落在已发展的波浪段 |
| `referenceTheta` | OF→HOS 绕 z 旋转角（度），斜浪对齐 | 与 HOS-NWT `theta_mono` 一致 |
| `Grid2GridInputFileName` | Grid2Grid 字典文件名（在 `constant/` 下） | 默认 `Grid2Grid.dict` |

### 3.1 坐标映射公式（θ=0 时）

```text
HOS_x = OF_x + referencePosition.x
HOS_y = OF_y + referencePosition.y
HOS_z = OF_z - seaLevel                  // seaLevel 来自 waveProperties
HOS_t = OF_t + referenceTime
```

有旋转时先按 `referenceTheta` 在水平面旋转，再平移。

### 3.2 为什么 `referenceTime` 必须为 0

官方 `Grid2Grid::step()` 用**原始 OF 时间**（不带 referenceTime）更新
Grid2Grid 内部时刻索引，而 `eta()/U()` 查询用的是 `time + referenceTime`。
两者不一致会导致样条时间窗错位（报 “y/z value out of bounds” 或取到错误
时刻）。因此本仓库所有算例都设 `referenceTime 0`。

## 4. 参数间的约束关系

1. **`zMax > 0`、`zMin` 覆盖水深**：z 重构网格必须从水底伸到自由面上方。
2. **2D（n2=1）时 y 方向只有 y=0 有解**：CFD 网格 y 必须单层且中心在
   y=0；`referencePosition.y` 保持 0。
3. **时间覆盖**：HOS-NWT `T_stop ≥ referenceTime + CFD endTime`（本仓库
   referenceTime=0，所以 `T_stop ≥ endTime`）。mode 文件必须覆盖 CFD 全程。
4. **波浪传播**：波以相速度传播，耦合区要等波传到才有值。CFD `endTime`
   要覆盖“波到达耦合区 + 若干周期”。
5. **时间分辨率**：HOS-NWT `f_out` 决定 mode 文件时间步，每周期至少
   10~20 个数据点；`interpolationOrder` 决定时间样条窗口
   （`nSaveT = interpolationOrder + 3`）。
6. **垂向分辨率**：`nZMin/nZMax` 与水深/波长匹配；浅水长波可少，短波
   多。`zMeshType=meshratio` 可在波面附近加密。
7. **HOS 与 CFD 尺寸**：HOS 水槽只需覆盖“耦合区 + 造波机近场”即可，
   不必等于 CFD 全域（见 `referencePosition` 用法）。
8. **重力**：必须沿 -z（`Grid2Grid::mappingTensors` 会检查）。

## 5. SWENSE mode 文件（Grid2Grid 的输入）

HOS-NWT 用 `i_sw=1` 输出 `Results/modes_HOS_SWENSE.dat`。文件是定长记录
（direct access）的 ASCII，记录 1 头部 9 个数值：

```text
n1   n3_add   n2   1/f_out   T_stop   xlen   ylen   h   0（校验位）
```

Grid2Grid 读取后换算：长度尺度 `dimL = h`，时间尺度
`dimT = sqrt(h/g)`，内部全部转成物理单位。因此：

* mode 文件里的 `xlen/ylen/h` 必须与 HOS 水池一致；
* 第 9 个校验位必须是 0，否则报
  “Input file is not HOS NWT Result File”；
* 给 Grid2Grid 的是 **SWENSE 文件**，不是 `HOS_modes.dat`
  （Tecplot 格式，`imodes=1` 输出）。

## 6. 使用流程与参数核对清单

```bash
# ① 造波：改 HOS-NWT input_HOS-NWT.dat（amp/nu 或 Hs/Tp/...、T_stop、i_sw=1）
bash HOSwaves2Foam/tutorials/HOS-NWT/runHOS-NWT.sh
cp Results/modes_HOS_SWENSE.dat <case>/constant/

# ② 配 Grid2Grid.dict：Grid2Grid 类型、filePath、zMesh、HOS 块参数
# ③ 配 waveProperties：externalForcing Grid2Grid + Grid2GridCoeffs
# ④ controlDict 加 libs ("libwaves2FoamGrid2Grid")
# ⑤ 运行
bash <case>/Allrun
```

核对清单：

- [ ] `i_sw = 1`，mode 文件是 SWENSE 格式且校验位为 0
- [ ] `Grid2Grid.dict` 顶层类型与 mode 文件来源一致（HOSNWT/HOSOcean）
- [ ] `filePath` 路径正确（相对算例目录）
- [ ] `zMin/zMax` 覆盖水柱且 `zMax > 0`
- [ ] `n2=1` 时 CFD 网格 y 单层、中心 y=0
- [ ] `referenceTime = 0`，`referencePosition` 使耦合区落在 HOS 有效区
- [ ] HOS-NWT `T_stop` 覆盖 CFD `endTime`
- [ ] 重力 `(0 0 -9.81)`
- [ ] `controlDict` 已加载 `libwaves2FoamGrid2Grid`
- [ ] `setWaveField` 日志出现 `GRID2GRID Vol2Vol is initialized`

## 7. 常见报错与对应参数

| 报错 | 相关参数 |
|---|---|
| `zMax should be positive` | `zMesh.zMax ≤ 0`，改为 > 0 |
| `zMinRatio/zMaxRatio should be larger than 0` | `zMeshType=meshratio` 时比例参数 ≤ 0 |
| `Bad value during floating point read` | 文件不是 SWENSE 格式；检查 `i_sw=1` 与 `filePath` |
| `Input file is not HOS NWT Result File` | 文件头校验位非 0/格式不符 |
| `Wrong solver is given` | `type` 不是 `NWT`/`Ocean` |
| `Wrong reconstruction procedure` | `procedure` 不是 `velocity`/`potential` |
| `y value out of bounds`（2D 样条） | 多为时间窗错位：检查 `referenceTime=0`、`T_stop` 覆盖 |
| `z value out of bounds`（3D 样条） | 查询 z 超出 `zMin..zMax`：扩大 zMesh 范围 |
| `simulTime is over endTime` | mode 数据时长不够：加大 HOS-NWT `T_stop` |
| `Unknown type of external wave forcing: Grid2Grid` | `controlDict` 没加 `libs` 或耦合库未编译 |

## 8. 与 HOS-NWT 参数的对应关系

| HOS-NWT 输入 | 作用 | 对应 Grid2Grid/CFD 参数 |
|---|---|---|
| `h` | 水深 | `zMesh.zMin`、`waveProperties.depth`、CFD 网格 |
| `amp_mono`/`Hs` | 波高 | 无（自动从 mode 读取） |
| `nu_mono`/`Tp` | 周期 | 无（自动）；CFD `endTime` 要够长 |
| `T_stop` | 数据时长 | 必须 ≥ CFD `endTime`（+referenceTime） |
| `f_out` | 数据时间分辨率 | `interpolationOrder` 的时间窗 |
| `theta_mono` | 波向 | `Grid2GridCoeffs.referenceTheta` |
| `i_sw` | 是否输出 SWENSE | `filePath` 指向的文件 |
