# overWaveDyMFoam

`overWaveDyMFoam` is a custom OpenFOAM solver that combines the overset-mesh capability of `overInterDyMFoam` with the wave-generation and relaxation-zone functionality used by `waves2Foam`.

`overWaveDyMFoam` 是一个基于 OpenFOAM 的定制求解器，用来把 `overInterDyMFoam` 的重叠网格能力与 `waves2Foam` 的造波和松弛区功能结合起来。

## What This Solver Does

This solver is intended for two-phase free-surface simulations where you want to use:

- overset mesh / overlapping mesh
- dynamic mesh motion
- VOF interface capturing
- relaxation-zone wave generation from `waves2Foam`
- external wave forcing support

它主要面向以下这类两相自由液面问题：

- 需要使用重叠网格
- 需要动态网格运动
- 使用 VOF 捕捉气液界面
- 需要 `waves2Foam` 的松弛区造波能力
- 需要外部波浪强迫功能

## Solver Origin

The code is based on:

- `overInterDyMFoam` from OpenFOAM
- `waveFoam` and related wave libraries from `waves2Foam`

In other words, this repository is a practical merged solver for users who need both overset mesh and wave-generation features in the same case.

也就是说，这个仓库的目标很直接：
把 OpenFOAM 里的重叠网格两相流思路，和 `waves2Foam` 里的造波能力，合并到同一个求解器里。

## Target Environment

From the current `Make/options`, this solver is configured for an environment close to:

- OpenFOAM `v2206+`
- `waves2Foam`
- `waves2FoamGABC`
- `waves2FoamSampling2206`
- GSL

The build options currently contain:

```text
-DOFVERSION=2206
-DOFPLUSBRANCH=1
```

so this repository is not a generic drop-in solver for every OpenFOAM version. It is aimed at a `v2206+` style setup with matching `waves2Foam` libraries already available.

从当前编译选项来看，这个版本并不是“所有 OpenFOAM 版本通用”，而是更偏向：
`OpenFOAM v2206+ + 对应 waves2Foam 环境` 的使用方式。

## Main Dependencies

The solver depends on:

- OpenFOAM finite-volume and multiphase libraries
- overset library
- dynamic mesh libraries
- `waves2Foam`
- `waves2FoamGABC`
- `waves2FoamSampling`
- GSL / GSL CBLAS

If these libraries are not correctly configured in your environment, the solver will not compile.

## Repository Structure

```text
overWaveDyMFoam.C   main solver
UEqn.H              momentum equation
pEqn.H              pressure equation
createFields.H      field creation and wave-related objects
createDyMControls.H dynamic-mesh related controls
Make/files          wmake source entry
Make/options        include paths and linked libraries
```

## Key Additions Compared with `overInterDyMFoam`

Some notable additions visible in the source include:

- `relaxationZone.H`
- `externalWaveForcing.H`
- `readWaveProperties.H`
- `createExternalWaveForcing.H`
- wave-related gravity/reference handling
- overset-related `oversetAdjustPhi`

These indicate that the solver is not just a rename, but a working integration of overset and wave modules.

## Installation

This solver is usually intended to be placed inside a `waves2Foam` solver tree rather than compiled as a completely standalone project.

Typical location:

```text
waves2Foam/applications/solvers/solvers2206_PLUS/overWaveDyMFoam
```

or replacing / extending the existing solver collection used by your local `waves2Foam` setup.

## Build

After putting the folder into the proper `waves2Foam` solver path, rebuild with the `waves2Foam` build system.

Typical workflow:

```bash
cd $WAVES_SRC/..
./Allwmake
```

If you only want to compile this solver directly and your environment is already configured correctly, you can also try:

```bash
wmake
```

However, for most users of this repository, rebuilding from the `waves2Foam` side is the safer approach because the include paths and linked libraries are tightly coupled to that environment.

## Usage Notes

Before compiling or running, make sure:

- OpenFOAM is sourced
- `waves2Foam` is correctly installed
- `WAVES_SRC`, `WAVES_LIBBIN`, `WAVES_GSL_INCLUDE`, and `WAVES_GSL_LIB` are available
- your case is prepared for overset mesh and wave-zone settings

This repository does not include a ready-to-run tutorial case yet, so users are expected to adapt an existing:

- `overInterDyMFoam` case
- or `waves2Foam/waveFoam` case

and then merge the relevant mesh, boundary, and wave settings.

## Recommended README-Level Expectation

This project is best understood as:

- a personal merged solver
- targeted at a specific OpenFOAM/waves2Foam environment
- useful for researchers or engineers who already know both overset and wave-generation workflows

It is not yet packaged as a one-click general-distribution solver.

## Chinese Summary

这个求解器可以理解为：

- 以 `overInterDyMFoam` 为基础
- 加入 `waves2Foam` 的松弛区造波与相关波浪模块
- 用于重叠网格 + 两相流 + 自由液面 + 波浪作用的联合模拟

推荐的使用方式是：

1. 放到你当前 `waves2Foam` 的求解器目录中
2. 确保 OpenFOAM 和 `waves2Foam` 环境变量已经加载
3. 通过 `waves2Foam` 的 `Allwmake` 重新编译
4. 基于已有 `waveFoam` 或 `overInterDyMFoam` 算例进行修改

## Current Status

At the current stage, this repository mainly provides:

- source code for the merged solver
- build files
- a lightweight usage explanation

Future improvements that would make this repository easier for other users:

- add a tutorial case
- add tested version notes
- add known limitations
- add a case-setup guide for overset + waves

## Acknowledgement

This project builds on the OpenFOAM ecosystem and the `waves2Foam` project. Credit belongs to the original upstream developers of those tools; this repository focuses on practical integration work for a specialized workflow.
