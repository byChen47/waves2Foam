# HOSwaves2Foam 教程：总运行手册

所有教程算例都遵循同一个两步流程：**先 HOS 造波，再 OpenFOAM 计算**。
本文件说明：①从零开始怎么装、②每次运行从哪一步开始、③通用验证与清理。
每个算例目录里还有自己的 `README.md`（波浪参数逐项解释）。

## 1. 算例一览与“从哪一步开始”

| 算例目录（HOS 造波 → CFD） | 波浪 | 数据已内置 | 需要的 HOS-NWT | 日常开始步骤 |
|---|---|---|---|---|
| `HOS-NWT/` → `waveFoam-HOS/` | 规则波 A=0.05 m, T=2 s | 是（17 MB） | 2D `build/` | 第 4 步 |
| `HOS-NWT-Irregular/` → `waveFoam-HOS-Irregular/` | 不规则波 JONSWAP | 是（59 MB） | 2D `build/` | 第 4 步 |
| `stokesI-HOS/`（含 `HOS-NWT/`） | 深水规则波 H=0.05 m, T=3 s | 是（88 MB） | 2D `build/` | 第 4 步 |
| `stokesI-HOS-3D/`（含 `HOS-NWT/`） | 三维斜波 15°, n2=8 | 是（89 MB） | 3D `build3D/` | 第 4 步 |

**从哪一步开始（决策）**

* **第一次使用**：从下面的「0. 环境」和「1. 一次性编译」开始；
* **只想跑现成算例**（数据已内置）：直接进对应 CFD 算例目录，从「4. CFD 运行」开始；
* **想改波高 / 周期 / 谱型 / 波向**：从「3. HOS 造波」开始，把新数据拷进 CFD 算例后再做「4. CFD 运行」。

## 0. 环境（每个新终端都要做）

```bash
source $HOME/OpenFOAM/OpenFOAM-v2206/etc/bashrc
# 若 WAVES_DIR 未设置：先 cd 到 waves2Foam 根目录，再执行
source $WAVES_DIR/bin/bashrc
```

## 1. 一次性编译

```bash
# Grid2Grid 数据桥库（安装到 $FOAM_USER_LIBBIN）
bash HOSwaves2Foam/scripts/buildGrid2Grid.sh

# waves2Foam 的 Grid2Grid 耦合库
cd HOSwaves2Foam/src/Grid2Grid && wmake libso

# HOS 波浪模型（2D 教程需要）
bash HOSwaves2Foam/scripts/buildHOS.sh

# 三维算例额外需要（n1=128, n2=8，编到 build3D/，不覆盖 2D 版本）
bash HOSwaves2Foam/scripts/buildHOS3D.sh
```

> 等价做法：在 waves2Foam 根目录直接 `./Allwmake` 会自动完成上面的
> `buildGrid2Grid.sh`、`buildHOS.sh` 和耦合库编译（3D 的
> `buildHOS3D.sh` 除外，需手动执行一次）。

## 2. 通用步骤结构（每个算例相同）

```text
第 3 步：HOS 造波（只有改波浪参数才需要）
         → <造波目录>/Results/modes_HOS_SWENSE.dat
         → cp 到 CFD 算例 constant/
第 4 步：CFD 运行（日常入口）
         → cd <CFD 算例> && bash ./Allrun
```

`Allrun` 内部依次执行：

```text
restore0Dir（把 0.orig 恢复为 0/）
→ blockMesh（建网格）
→ setWaveField（初始化波浪场，同时初始化 Grid2Grid）
→ waveGaugesNProbes（生成波高测点函数对象）
→ waveFoam（求解）
```

## 3. HOS 造波（仅改波浪参数时需要）

```bash
# 2D 算例（规则波 / 不规则波 / stokesI-HOS）
cd HOSwaves2Foam/tutorials/<造波目录>       # 如 HOS-NWT、HOS-NWT-Irregular、
                                             # stokesI-HOS/HOS-NWT
bash runHOS-NWT.sh                          # 自动找 src/HOS-NWT/build/HOS_NWT

# 3D 算例（stokesI-HOS-3D）
cd HOSwaves2Foam/tutorials/stokesI-HOS-3D/HOS-NWT
bash runHOS-NWT.sh                          # 自动找 build3D/HOS_NWT

# 把新数据交给 CFD 算例
cp Results/modes_HOS_SWENSE.dat <CFD 算例>/constant/
```

改波浪参数的位置：

* 规则波：`input_HOS-NWT.dat` 的 `amp_mono`（振幅=波高/2）、`nu_mono`（频率，T=1/nu）；
* 不规则波：`Hs`、`Tp`、`gamma`、`iseed`；
* 波向/三维：`theta_mono`（>0 为斜波）；3D 还需 `buildHOS3D.sh` 的 n2>1；
* 时长：`T_stop` 必须覆盖 CFD 的 `endTime`。

## 4. CFD 运行（日常入口）

```bash
cd HOSwaves2Foam/tutorials/<CFD 算例>       # 如 waveFoam-HOS、waveFoam-HOS-Irregular、
                                             # stokesI-HOS、stokesI-HOS-3D
bash ./Allrun
```

> 注意用 `bash ./Allrun`（带 `./`）。直接 `bash Allrun` 会因脚本里
> `cd "${0%/*}"` 取不到目录而报错。

## 5. 验证 / 停止 / 重跑 / 清理

**验证是否正常运行**

* `log.setWaveField` 里出现：
  ```text
  HOS Wrapper Program GRID2GRID Vol2Vol is initialized.
  HOS Result File : constant/modes_HOS_SWENSE.dat
  ```
* 算例根目录出现时间目录 `0, 0.5, 1, ...`（到 `controlDict` 的 `endTime`）；
* `postProcessing/surfaceElevationAnyName/0/surfaceElevation.dat` 有波高时序
  （waveGaugesNProbes 输出，列 = 各测点，行 = 时间）；
* 想查看 Grid2Grid 重构探针（多 (x,y) 点）：在 CFD 算例目录运行
  `postG2G constant/Grid2Grid.dict`，得到 `waveElevation.dat` 等。

**停止 / 重跑 / 清理**

```bash
# 中途停止：Ctrl-C
# 重跑：直接再次 bash ./Allrun（会先 restore0Dir，从时间 0 开始）
# 彻底清理本算例的生成物：
./Allclean
# 清理全部（waves2Foam + HOS 构建产物）：
cd <waves2Foam 根> && ./Allwclean
```

## 6. 各算例详细说明

| 算例 | 说明 |
|---|---|
| [HOS-NWT/README.md](HOS-NWT/README.md) | 规则波造波（第 1 步），含 `input_HOS-NWT.dat` 逐项解释 |
| [waveFoam-HOS/README.md](waveFoam-HOS/README.md) | 规则波 CFD 算例，波浪配置文件逐行解释 |
| [HOS-NWT-Irregular/README.md](HOS-NWT-Irregular/README.md) | 不规则波造波（JONSWAP 参数） |
| [waveFoam-HOS-Irregular/README.md](waveFoam-HOS-Irregular/README.md) | 不规则波 CFD 算例 |
| [stokesI-HOS/README.md](stokesI-HOS/README.md) | 大型深水水池（2D） |
| [stokesI-HOS-3D/README.md](stokesI-HOS-3D/README.md) | 大型三维水池（斜波、n2=8） |

更完整的参数说明见 [../doc/使用说明.md](../doc/使用说明.md) 与
[../doc/HOS-Grid2Grid参数详解.md](../doc/HOS-Grid2Grid参数详解.md)。
