# HOS 势流模型源码：HOS-NWT 与 HOS-ocean

本目录存放两个 **Higher-Order Spectral (HOS)** 势流波浪模型的完整源码，
它们是本仓库 HOS 耦合链路（HOS → Grid2Grid → waves2Foam 松弛区）的
**波浪数据来源**：

```text
src/
├── HOS-NWT/    数值波浪水池（造波机、反射墙、数值消波滩）→ 教程算例使用
├── HOS-ocean/  开放海域波浪传播（周期边界）
├── Grid2Grid/  waves2Foam 耦合库（libwaves2FoamGrid2Grid.so）源码
```

两个模型均由 **LHEEA（法国南特中央理工 / CNRS）** 开发并开源，原始上游
见 <https://github.com/LHEEA/HOS-NWT> 与 <https://github.com/LHEEA/HOS-ocean>
（已迁移至 GitLab）。随本仓库分发的版本**已经包含 gfortran 9 / 现代
工具链的修复补丁**（见下文“已包含的修复”），可直接编译。

## 1. 前置依赖

编译 HOS 模型只需要：

* gfortran（Fortran 编译器）
* cmake（≥ 2.8.8，建议用系统自带版本）

**FFTW3 和 LAPACK/BLAS 已经内置在 `HOSwaves2Foam/ThirdParty/`**
（见 [../ThirdParty/README.md](../ThirdParty/README.md)），无需系统安装。
Ubuntu 只需：`sudo apt install cmake gfortran`。

## 一键编译

```bash
bash HOSwaves2Foam/scripts/buildHOS.sh
```

脚本自动用仓库内置的 FFTW/LAPACK 编译 HOS-NWT 和 HOS-ocean，产物：

* `HOSwaves2Foam/src/HOS-NWT/build/HOS_NWT`
* `HOSwaves2Foam/src/HOS-ocean/build/HOS_ocean`

也可以手动执行（见下）。

## 2. 编译 HOS-NWT（数值波浪水池，教程用这个）

```bash
# 每次新终端先加载 OpenFOAM 环境（只是为了拿到通用的构建工具链约定，
# HOS-NWT 本身不依赖 OpenFOAM）
source $HOME/OpenFOAM/OpenFOAM-v2206/etc/bashrc

cd $WAVES_DIR/HOSwaves2Foam/src/HOS-NWT

cmake -Hcmake -Bbuild \
      -DCMAKE_BUILD_TYPE=Release \
      -DFFTW_ROOT=$PWD/../../ThirdParty/fftw3 \
      -DCMAKE_LIBRARY_PATH=$PWD/../../ThirdParty/lapack \
      -DCMAKE_EXE_LINKER_FLAGS=-L$PWD/../../ThirdParty/lapack

# 注意：-j1 单线程构建，避免 gfortran 模块文件（.mod0）改名竞争
cmake --build build -j1
```

产物：`build/HOS_NWT`（可执行文件）。

验证编译（官方 Benchmark，可选）：

```bash
cd Benchmark/Regular
mkdir -p Results
../../build/HOS_NWT
```

运行自己的造波案例（本仓库教程）：

```bash
cd $WAVES_DIR/HOSwaves2Foam/tutorials/HOS-NWT
HOSNWT_BIN=$WAVES_DIR/HOSwaves2Foam/src/HOS-NWT/build/HOS_NWT \
    bash runHOS-NWT.sh
```

产出 `Results/modes_HOS_SWENSE.dat`，拷到 waveFoam-HOS 算例：

```bash
cp Results/modes_HOS_SWENSE.dat ../waveFoam-HOS/constant/
```

## 3. 编译 HOS-ocean（开放海域）

与 HOS-NWT 完全相同的流程：

```bash
cd $WAVES_DIR/HOSwaves2Foam/src/HOS-ocean

cmake -Hcmake -Bbuild \
      -DCMAKE_BUILD_TYPE=Release \
      -DFFTW_ROOT=$PWD/../../ThirdParty/fftw3 \
      -DCMAKE_LIBRARY_PATH=$PWD/../../ThirdParty/lapack \
      -DCMAKE_EXE_LINKER_FLAGS=-L$PWD/../../ThirdParty/lapack

cmake --build build -j1
```

产物：`build/HOS_ocean`（主程序）、`build/Post_processing`、
`build/check_W_DYue`（Benchmark 校验程序）。

HOS-ocean 的输入配置在 `bin/input_HOS.dat`；用 HOS-ocean 造波时：

1. 在案例目录放好 `input_HOS.dat` 并运行 `build/HOS_ocean`；
2. 得到 HOS-ocean 的 mode 文件；
3. `waveFoam-HOS/constant/Grid2Grid.dict` 里把 `Grid2Grid HOSNWT;`
   改成 `Grid2Grid HOSOcean;`，并把 `HOSOcean.filePath` 指向该 mode 文件；
4. 其余流程不变。

## 4. 已包含的修复补丁（重新从上游拉取时需要重打）

源码里已经改好，**直接用本目录即可，无需再打**：

### HOS-NWT（`src/HOS-NWT/`）

1. `cmake/cmake/CMakeCompilationOptionsFortran.cmake`
   —— 去掉 gfortran 的 `-fno-automatic`（gfortran 9 下会导致
   `SAVE attribute cannot be specified in a PURE procedure` 编译错误），
   并去掉 `-pedantic-errors -Werror` 等过严选项；
2. `sources/main/dealiasing.f90`
   —— `MOD(order-1, p2-1)` 在 `p2=1` 时除数为编译期常量 0，gfortran 9
   直接报错；改用带 `m /= 0` 保护的 `isDivisible(n, m)` 辅助函数。

### HOS-ocean（`src/HOS-ocean/`）

1. `cmake/CMakeLists.txt`
   —— `PROJECT(HOS_ocean C)` 改为 `PROJECT(HOS_ocean C Fortran)`：
   FindBLAS 的链接测试需要 Fortran 运行时（`-lgfortran`），否则报
   “Could NOT find BLAS”；
2. `cmake/cmake/CMakeCompilationOptionsFortran.cmake`
   —— 与 HOS-NWT 相同的 `-fno-automatic` / 过严选项修复。

## 5. 常见问题

| 现象 | 解决 |
|---|---|
| `Could NOT find BLAS` | 用 `CMAKE_LIBRARY_PATH` 指向 `HOSwaves2Foam/ThirdParty/lapack`（已内置）；HOS-ocean 还需 `PROJECT(... C Fortran)` 补丁（已含） |
| gfortran `.mod0` 改名失败 / `Cannot rename module file` | `mkdir -p build/lib` 并用 `-j1` 单线程构建 |
| `SAVE attribute ... PURE procedure` | 去掉 `-fno-automatic`（已含） |
| `Argument 'P' of MOD ... shall not be zero` | 打 dealiasing 除零保护补丁（已含） |
| 链接找不到 `-llapack/-lblas` | 加 `-DCMAKE_EXE_LINKER_FLAGS=-L<目录>` |

## 6. 说明

* `build/` 目录建议建在源码树内（上面的命令就是这样），已加入仓库
  `.gitignore`，不会被提交；
* 编译依赖（FFTW3/LAPACK/BLAS）已内置在 `HOSwaves2Foam/ThirdParty/`，
  可离线编译；
* 这两个模型是独立的第三方软件（GPL），与 waves2Foam 的耦合通过
  Grid2Grid（`src/Grid2Grid` 对应 `libwaves2FoamGrid2Grid.so`）完成；
* 详细的使用与波浪场设置见
  [../doc/使用说明.md](../doc/使用说明.md)。
