# ThirdParty：HOS 模型编译依赖（已内置）

这里存放编译 HOS-NWT / HOS-ocean 所需的第三方库，全部**随仓库分发**，
不需要再用 `apt` 等系统包管理器安装：

```text
ThirdParty/
├── fftw3/     FFTW3 头文件 + 静态库（double/single/long-double）
└── lapack/    LAPACK + BLAS 静态库
```

## fftw3

* 来源：系统 FFTW3（Ubuntu `libfftw3-dev`，版本 3.3.8）
* 内容：`fftw3.h`、`fftw3.f03`（HOS 源码 `INCLUDE 'fftw3.f03'` 需要）、
  `libfftw3.a` / `libfftw3f.a` / `libfftw3l.a`（静态库，链接后运行时不依赖
  系统 FFTW 共享库）、`copyright`（GPLv2+ 许可声明）
* 用途：HOS 模型做 FFT 变换（`fourier_FFTW.f90` 等）

## lapack

* 来源：waves2Foam 自带的 LAPACK 3.3.1（`$WAVES_DIR/ThirdParty/lapack-3.3.1.tgz`，
  由 `ThirdParty/Allwmake` 用 gfortran 编译）
* 内容：
  * `liblapack.a`（即 `ThirdParty/lib/liblapack_gfortran.a` 的副本）
  * `libblas.a`（`ThirdParty/lib/libblas.a` 的副本）
* 用途：HOS 模型解线性方程组（`LSAME`、`DGESV` 等例程）
* 许可：LAPACK 3.x 采用类 BSD 许可，见上游
  `ThirdParty/lapack-3.3.1/LICENSE`（waves2Foam 内置源码）

> 说明：静态库是为 x86-64 Linux + gfortran 构建的；换架构/编译器时需要
> 从 `lapack-3.3.1.tgz` 重新编译（waves2Foam 的 ThirdParty 流程已支持），
> FFTW3 可从 https://www.fftw.org 获取源码自行编译。

## 怎么用

一键编译两个 HOS 模型（只依赖 vendored 库 + 系统里的 cmake/gfortran）：

```bash
bash HOSwaves2Foam/scripts/buildHOS.sh
```

等价的手动命令（以 HOS-NWT 为例）：

```bash
cd HOSwaves2Foam/src/HOS-NWT
cmake -Hcmake -Bbuild -DCMAKE_BUILD_TYPE=Release \
      -DFFTW_ROOT=$PWD/../../ThirdParty/fftw3 \
      -DCMAKE_LIBRARY_PATH=$PWD/../../ThirdParty/lapack \
      -DCMAKE_EXE_LINKER_FLAGS=-L$PWD/../../ThirdParty/lapack
mkdir -p build/lib
cmake --build build -j1
```
