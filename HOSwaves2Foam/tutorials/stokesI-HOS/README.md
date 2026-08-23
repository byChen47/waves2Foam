# stokesI-HOS：大型水池 HOS 造波算例（由 StokesI 改造）

## 这个算例做什么

一个 **624 m（长）× 200 m（高）** 的大型二维水池（水深 100 m、y 方向单层），
演示把原来的 **Stokes-I**（IHFoam `waveModel StokesI`，H = 0.05 m、
T = 3 s）造波设置改造成 **HOS 造波**（HOS-NWT → Grid2Grid → waves2Foam
松弛区），波浪参数等价：振幅 0.025 m、周期 3 s。

入口松弛区在 x = -312..-282 m 注入 HOS 波浪，出口松弛区
x = 282..312 m 消波，模拟 30 s。

## 运行顺序（两步流程）

```text
第 1 步（可选）：HOS-NWT 造波（HOS-NWT/ 子算例，生成 modes_HOS_SWENSE.dat）
第 2 步：bash ./Allrun（CFD 计算）
```

`constant/modes_HOS_SWENSE.dat` 已内置（88 MB），直接跑第 2 步即可。

## 目录结构

```text
stokesI-HOS/
├── 0.orig/                  # 初始场模板
├── HOS-NWT/                 # HOS-NWT 造波子算例（生成 SWENSE 数据）
│   ├── input_HOS-NWT.dat    #   200 m 水池、100 m 水深、A=0.025、T=3 s
│   └── runHOS-NWT.sh
├── constant/
│   ├── waveProperties       # ★ 波浪配置（已改为 Grid2Grid）
│   ├── Grid2Grid.dict       # ★ Grid2Grid 数据桥配置
│   ├── modes_HOS_SWENSE.dat # ★ HOS 波浪数据（已内置，88 MB）
│   ├── g                    # 重力 (0 0 -9.81)
│   └── polyMesh/blockMeshDict
├── system/
│   ├── controlDict          # 时间步、库加载
│   ├── blockMeshDict        # 624 m × 200 m 网格（156×1×33）
│   ├── topoSetDict.* / refineMeshDict.*  # 可选网格加密
│   └── decomposeParDict     # 8 进程并行配置
└── Allrun                   # 一键运行（加密/并行默认注释）
```

## 运行步骤

前置：`libGrid2Grid.so`、`libwaves2FoamGrid2Grid.so` 已编译；环境已加载。

```bash
cd HOSwaves2Foam/tutorials/stokesI-HOS
bash ./Allrun          # restore0Dir → blockMesh → setWaveField → waveFoam
```

> 基础网格 156×1×33 的竖向单元约 6 m，**分辨率不足以解析 0.05 m 的波面**。
> 生产计算请打开 `Allrun` 里被注释的 4 轮 `topoSet`/`refineMesh`（单元数约
> ×256），再配合 `decomposePar` + `mpirun -np 8 waveFoam -parallel`。

重新生成 HOS 波浪数据：

```bash
cd HOS-NWT
bash runHOS-NWT.sh
cp Results/modes_HOS_SWENSE.dat ../constant/
```

## 波浪配置文件详解

### `constant/waveProperties`

与 `waveFoam-HOS` 结构相同（`externalSource` + 两个松弛区 +
`externalForcing Grid2Grid`），本算例的特殊取值：

```cpp
inletCoeffs
{
    waveType    externalSource;
    Tsoft       3;              // 斜坡 3 s
    depth       100.0;          // 水深 100 m（与 HOS-NWT、网格一致）
    relaxationZone
    {
        relaxType   INLET;
        startX      ( -312 0 -100 );  // 松弛区覆盖整个水柱（z -100..5）
        endX        ( -282 0 5 );
        orientation ( 1.0 0.0 0.0 );
    }
};

outletCoeffs
{
    waveType    potentialCurrent;    // 出口消波
    U           ( 0 0 0 );
    Tsoft       3;
    relaxationZone { relaxType OUTLET; startX (312 0 -100); endX (282 0 5); ... }
};

Grid2GridCoeffs
{
    Tsoft               3;
    referenceTime       0;          // 保持 0（见 waveFoam-HOS README）
    referencePosition   ( 332 0 0 ); // ★ OF x=-312 → HOS x=20（远离造波机近场）
    referenceTheta      0;
    Grid2GridInputFileName Grid2Grid.dict;
}
```

`referencePosition (332 0 0)` 的含义：HOS 坐标 = OF 坐标 + 332，于是
CFD 入口区 OF x=-312..-282 对应 HOS 水槽 x=20..50——取已发展的波浪段。

### `constant/Grid2Grid.dict`

与 `waveFoam-HOS` 相同，只是 z 网格覆盖整个 100 m 水柱：

```text
zMesh
{
    zMin           -100.0;   // 水底
    zMax            5.0;     // 自由面上方空气余量（必须 > 0）
    nZMin           33;      // 水面以下节点数
    nZMax           15;      // 水面以上节点数
    zMeshType         uniform;
}
HOSNWT { filePath constant/modes_HOS_SWENSE.dat; procedure velocity; ... }
```

### `HOS-NWT/input_HOS-NWT.dat`（波浪参数）

```text
xlen       200.0      // HOS 水池长（比 CFD 的 624 m 短没关系，只覆盖耦合区）
h          100.0      // 水深，与 CFD 一致
amp_mono   0.025      // 振幅 = 波高/2（原 StokesI 波高 0.05 → 振幅 0.025）
nu_mono    0.3333333333  // 频率 Hz，T = 3 s
igeom      1          // 活塞式造波机（浅水/深水都稳定）
T_stop     60.0       // 造波 60 s，覆盖 CFD 30 s 全程
i_sw       1          // SWENSE 输出（Grid2Grid 必需）
```

### `system/blockMeshDict`（几何要点）

```text
xmin -312  xmax 312   // 624 m
ymin -0.01 ymax 0.01  // y 单层，中心在 y=0（Grid2Grid n2=1 要求）
zmin -100  zmax 100   // 200 m 高（100 m 水 + 100 m 空气）
xnumber 156  ynumber 1  znumber 33
```

## 常见问题

* 不加密网格时波面看不出起伏是正常的（竖向 6 m 单元），先确认日志里
  `GRID2GRID Vol2Vol is initialized` 且入口区 U 非零，再开加密。
* 重力必须是 `(0 0 -9.81)`（`constant/g` 已设置）。
