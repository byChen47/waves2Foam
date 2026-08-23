# waveFoam-HOS：规则波 CFD 耦合算例

## 这个算例做什么

一个 18 m 长的二维 `waveFoam` 水槽（0.4 m 水深 + 0.1 m 空气，y 方向单层），
演示 **HOS-NWT 规则波 → Grid2Grid → waves2Foam 松弛区** 的完整链路：
入口松弛区（x=0..5 m）把 HOS 波浪场注入 CFD，出口松弛区（x=13..18 m）
吸收波浪防止反射。模拟 8 s，入口区出现振幅约 0.05 m、周期 2 s 的规则波。

## 目录结构

```text
waveFoam-HOS/
├── 0.org/                 # 初始场模板（Allrun 会生成 0/）
├── constant/
│   ├── waveProperties     # ★ 波浪配置（waves2Foam 读这个）
│   ├── waveProperties.input  # 同内容存档（不用 setWaveParameters 时无实际作用）
│   ├── Grid2Grid.dict     # ★ Grid2Grid 数据桥配置
│   ├── modes_HOS_SWENSE.dat # ★ HOS-NWT 生成的波浪数据（已内置）
│   ├── g                  # 重力 (0 0 -9.81)
│   └── polyMesh/blockMeshDict  # 网格
├── system/controlDict     # 时间步、求解器、库加载
└── Allrun                 # 一键运行脚本
```

## 运行步骤

前置：`libGrid2Grid.so` 与 `libwaves2FoamGrid2Grid.so` 已编译安装
（见 `HOSwaves2Foam/README.md`）；OpenFOAM + waves2Foam 环境已加载。

```bash
cd HOSwaves2Foam/tutorials/waveFoam-HOS
bash ./Allrun          # 依次执行 blockMesh → setWaveField → waveFoam

# 查看结果（时间目录 0..8）
foamToVTK -time 8      # 用 ParaView 打开
```

`setWaveField` 时日志里出现 `GRID2GRID Vol2Vol is initialized` 和
`HOS Result File : constant/modes_HOS_SWENSE.dat` 说明耦合初始化成功。

## 波浪配置文件详解

### `constant/waveProperties`（逐行）

```cpp
seaLevel        0.00;      // 静水面 z 坐标（m）

relaxationNames (inlet outlet);   // 两个松弛区：入口造波、出口消波
initializationName outlet;        // 初始场按出口区（静水）初始化

inletCoeffs                      // 入口“造波区”
{
    waveType    externalSource;  // 波浪数据来自外部源（HOS）
    Tsoft       1;               // 前 1 s 波幅斜坡爬升，避免启动冲击
    depth       0.400000;        // 水深，与 HOS-NWT 的 h、网格一致

    relaxationZone
    {
        relaxationScheme Spatial;      // 空间松弛（沿 x 加权混合）
        relaxationShape  Rectangular;  // 矩形松弛区
        beachType        Empty;        // 无数值海滩
        relaxType   INLET;             // 入口型
        startX      ( 0 0 -1 );        // 松弛区起点
        endX        ( 5 0  1 );        // 终点（x 0..5 m）
        orientation ( 1.0 0.0 0.0 );   // 松弛方向沿 x
    }
};

outletCoeffs                     // 出口“消波区”
{
    waveType    potentialCurrent; // 静止势流（U=0），吸收波浪
    U           ( 0 0 0 );
    Tsoft       1;
    relaxationZone { ... relaxType OUTLET; startX (13 0 -1); endX (18 0 1); ... }
};

externalForcing  Grid2Grid;      // 选择外部波浪源 = Grid2Grid

Grid2GridCoeffs
{
    waveType            Grid2Grid;
    Tsoft               1;       // 斜坡时间（s）
    referenceTime       0;       // HOS 与 OF 时间轴对齐偏移（保持 0）
    referencePosition   ( 5 0 0 ); // OF x=0 → HOS x=5（避开造波机近场）
    referenceTheta      0;       // OF→HOS 旋转角（°）
    Grid2GridInputFileName Grid2Grid.dict; // Grid2Grid 配置文件名
}
```

**改波浪**：波高/周期在 HOS-NWT 侧改（`../HOS-NWT/input_HOS-NWT.dat` 的
`amp_mono`、`nu_mono`），重新 `runHOS-NWT.sh` 并把新的
`modes_HOS_SWENSE.dat` 拷进来，然后重跑本算例。

### `constant/Grid2Grid.dict`（逐行）

```text
Grid2Grid       HOSNWT;      // 数据来源：HOS-NWT（另一选项 HOSOcean）
writeVTK / write*Probe ...   // 诊断输出开关（本例关闭/未用）

zMesh                       // 垂向重构网格，覆盖整个耦合区水体 + 空气余量
{
    zMin           -0.4;     // 最底 = 水深
    zMax            0.1;     // 最顶（必须 > 0，伸到自由面上方）
    nZMin           21;      // 水面以下节点数
    nZMax           21;      // 水面以上节点数
    zMeshType         uniform;  // 均匀（或 meshRatio 近自由面加密）
}

HOSNWT
{
    filePath        constant/modes_HOS_SWENSE.dat; // mode 文件
    procedure       velocity; // 速度重构
    extraInterpolationNumber 2; // 谱模态插值加密倍数
    interpolationOrder  3;    // 三次样条
    $zMesh                     // 引用上面的 zMesh
    writeLog        TRUE;
}
```

### `system/controlDict`（关键项）

```cpp
endTime         8;        // 模拟时长：要让波传进松弛区并发展几个周期
deltaT          0.002;    // 初始步长
adjustTimeStep  yes;      // 自适应步长
maxCo           0.25;     // 库朗数上限
maxDeltaT       0.05;     // 步长上限
libs ( "libwaves2FoamGrid2Grid" );  // ★ 加载耦合库（忘了它会报
                                    //   “Unknown type ... Grid2Grid”）
```

## 常见问题

* 跑完发现水面全平：波还没传到耦合区，把 `endTime` 加大（HOS 数据
  `T_stop` 也要覆盖）。
* `zMax should be positive`：`Grid2Grid.dict` 的 `zMax` 必须 > 0。
* `y value out of bounds`：CFD 网格 y 必须单层且中心在 y=0（n2=1 时
  Grid2Grid 只在 y=0 有定义）。
