# waveFoam-HOS-Irregular：不规则波 CFD 耦合算例

## 这个算例做什么

与 `waveFoam-HOS` 相同的小型 18 m 水槽，但波浪源换成**不规则波**
（HOS-NWT 生成的 JONSWAP 谱，Hs = 0.05 m、Tp = 1 s）。入口松弛区
注入不规则波浪场，出口松弛区消波，模拟 20 s。

## 运行流程（从哪一步开始）

本算例数据已内置（`constant/modes_HOS_SWENSE.dat`），**日常直接做下面的
第 4 步 CFD 运行**；修改谱参数时才需要先做第 3 步造波。首次使用的
环境与编译步骤见 [tutorials/README.md](../README.md)。

```text
第 3 步（仅改参数时）：../HOS-NWT-Irregular 造波 → cp 新数据到 constant/
第 4 步（日常入口，本算例）：bash ./Allrun
```

## 目录结构

与 `waveFoam-HOS` 完全一致，只是：

* `constant/modes_HOS_SWENSE.dat` 是不规则波的 SWENSE 数据（已内置）；
* `system/controlDict` 的 `endTime = 20`、`writeInterval = 1`；
* `Allrun` 注释里指向不规则波造波算例 `../HOS-NWT-Irregular`。

## 运行步骤（第 4 步：CFD 运行）

```bash
cd HOSwaves2Foam/tutorials/waveFoam-HOS-Irregular
bash ./Allrun
```

重新生成不规则波数据（第 3 步，仅改谱参数时）：

```bash
cd ../HOS-NWT-Irregular
bash runHOS-NWT.sh
cp Results/modes_HOS_SWENSE.dat ../waveFoam-HOS-Irregular/constant/
cd ../waveFoam-HOS-Irregular && bash ./Allrun
```

验证：`log.setWaveField` 出现 `GRID2GRID Vol2Vol is initialized`。

## 波浪配置文件详解

与 `waveFoam-HOS/README.md` 第 3 节**完全相同**：

* `constant/waveProperties`：`externalSource` + 两个松弛区 +
  `externalForcing Grid2Grid` + `Grid2GridCoeffs`
  （`referencePosition (5 0 0)`、`referenceTime 0`）；
* `constant/Grid2Grid.dict`：`zMesh`（zMin -0.4、zMax 0.1）、
  `HOSNWT.filePath constant/modes_HOS_SWENSE.dat`；
* `system/controlDict`：`libs ("libwaves2FoamGrid2Grid")`。

直接参考那份 README 的逐行解释即可。区别只有波浪数据本身（不规则谱）。
