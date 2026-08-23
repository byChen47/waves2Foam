# waves2Foam – HOS Coupling Module

This folder implements the coupling chain

> **中文完整使用说明见 [doc/使用说明.md](doc/使用说明.md)** —— 包含逐行
> 配置解释、代码说明与波浪场设置方法。

```text
HOS potential-flow model (HOS-ocean / HOS-NWT)
        |
        |  mode files (free-surface elevation, velocity, dynamic pressure)
        v
Grid2Grid data bridge (LHEEA, Ecole Centrale de Nantes)
        |
        |  libGrid2Grid.so  (Fortran library)
        v
libwaves2FoamGrid2Grid.so  (Foam::waveTheories::Grid2Grid)
        |
        |  eta(x,t), U(x,t), pExcess(x,t)
        v
waves2Foam externalSource wave theory + relaxation zone
        |
        v
waveFoam / overWaveDyMFoam (CFD domain)
```

The coupling is implemented with the official Grid2Grid waves2Foam example
(`example/waves2Foam/` from the Grid2Grid repository, by Young-Myung Choi /
Bureau Veritas and LHEEA-ECN), adapted for OpenFOAM v2206.  It calls the
Grid2Grid Fortran library directly at run-time, which reconstructs the HOS
wave field on the fly at every query point, matching the
"HOS + Grid2Grid + relaxation zone" chain.

The **HOS wave models themselves (HOS-NWT and HOS-ocean) are vendored** in
`src/HOS-NWT` and `src/HOS-ocean` (patched for modern gfortran toolchains);
their build instructions are in [src/README.md](src/README.md).

## Folder layout

```text
HOSwaves2Foam/
├── README.md
├── doc/
│   ├── Implementation of Grid2Grid in waves2Foam.md   # official integration guide
│   └── HOS-Grid2Grid参数详解.md                        # parameter reference
├── src/
│   ├── HOS-NWT/          # vendored HOS-NWT source (numerical wave tank)
│   ├── HOS-ocean/        # vendored HOS-ocean source (open ocean)
│   ├── Grid2Grid/        # official Grid2Grid module  -> libwaves2FoamGrid2Grid.so
│   │   ├── Grid2Grid.H / Grid2Grid.C / Grid2GridFwd.H
│   │   └── Make/
│   └── Grid2GridProperties/  # optional setWaveProperties class (see doc/)
├── tutorials/
│   ├── waveProperties.input.Grid2Grid   # waveProperties template
│   ├── Grid2Grid.dict                   # Grid2Grid configuration template
│   ├── HOS-NWT/                         # HOS-NWT wave generation case
│   └── waveFoam-HOS/                    # waveFoam + Grid2Grid coupling case
└── scripts/
    ├── buildGrid2Grid.sh                # build + install libGrid2Grid.so
    └── buildHOS.sh                      # build HOS-NWT + HOS-ocean
```

## Official Grid2Grid coupling

### 1. Build libGrid2Grid.so

```bash
source $HOME/OpenFOAM/OpenFOAM-v2206/etc/bashrc
source $WAVES_DIR/bin/bashrc
bash HOSwaves2Foam/scripts/buildGrid2Grid.sh
```

This clones the Grid2Grid repository, configures it with
`-DBUILD_OF_LIB=ON` and installs `libGrid2Grid.so` into `$FOAM_USER_LIBBIN`
(FFTW3 and a Fortran compiler are required).  The script applies two build
fixes for modern gfortran: it creates the Fortran module directory and builds
single-threaded to avoid `.mod0` rename races.

### 2. Build the waves2Foam coupling library

```bash
cd HOSwaves2Foam/src/Grid2Grid
wmake libso
```

Produces `libwaves2FoamGrid2Grid.so` in `$FOAM_USER_LIBBIN`.  The class is
registered as `externalWaveForcing` type `Grid2Grid`.

### 3. Configure a case

* Copy `tutorials/waveProperties.input.Grid2Grid` to
  `<case>/constant/waveProperties` and adapt the relaxation zone and the
  `Grid2GridCoeffs`:

  ```c
  externalForcing  Grid2Grid;

  Grid2GridCoeffs
  {
      waveType            Grid2Grid;
      Tsoft               1;                 // ramp time [s]
      referenceTime       -20;               // HOS t=0 in OF time coordinate
      referencePosition   (-10 -10 0);       // HOS origin in OF coordinates
      referenceTheta      0;                 // rotation OF -> Grid2Grid [deg]
      Grid2GridInputFileName Grid2Grid.dict; // in <case>/constant
  }
  ```

* Copy `tutorials/Grid2Grid.dict` to `<case>/constant/Grid2Grid.dict`,
  select `HOSNWT` or `HOSOcean`, and set the HOS mode file path and the
  `zMesh` block (must cover the relaxation-zone water column).
* Add the library to `<case>/system/controlDict`:

  ```c
  libs ( "libwaves2FoamGrid2Grid" );
  ```

* Run the solver (`waveFoam` or `overWaveDyMFoam`).

### Notes

* The gravitational vector must point in **negative z** (checked by the
  module); the mesh must be generated accordingly.
* `referenceTime` aligns the Grid2Grid time axis with the OpenFOAM time
  axis, `referencePosition`/`referenceTheta` align the horizontal
  coordinates.
* `Tsoft` ramps the wave signal in to avoid an impulsive start.
* `Grid2GridProperties` (in `src/Grid2GridProperties/`) is the optional
  `setWaveProperties` class that lets `setWaveField`/`setWaveParameters`
  write the `Grid2GridCoeffs` automatically; per the official guide it is
  compiled into the waves2FoamProcessing library (see the vendored doc).
  The template above is equivalent and requires no core changes.

## Small tutorial cases

Four ready-to-run tutorial cases are provided:

> Each case directory contains its own `README.md` with the case purpose,
> run steps and a line-by-line explanation of the wave-setting files.

### `tutorials/HOS-NWT/` — HOS wave generation

A small HOS-NWT numerical wave tank (20 m x 1 m x 0.4 m, monochromatic
wave, piston wavemaker).  Run it with the HOS-NWT executable built from
<https://github.com/LHEEA/HOS-NWT> (see the build notes below):

```bash
cd HOSwaves2Foam/tutorials/HOS-NWT
HOSNWT_BIN=/path/to/HOS_NWT bash runHOS-NWT.sh
```

This writes `Results/modes_HOS_SWENSE.dat` (the **SWENSE** output, enabled
with `i_sw 1`), which is the file format consumed by Grid2Grid.  Note that
`Results/HOS_modes.dat` (the Tecplot-style mode output, `imodes 1`) is *not*
the format Grid2Grid reads.

### `tutorials/waveFoam-HOS/` — waves2Foam + Grid2Grid coupling

A small `waveFoam` flume (18 m x 0.1 m x 0.5 m, single cell in y) that reads
the HOS-NWT SWENSE modes through `libwaves2FoamGrid2Grid.so`:

```bash
cd HOSwaves2Foam/tutorials/waveFoam-HOS
bash Allrun
```

`constant/modes_HOS_SWENSE.dat` is already included (generated by the
HOS-NWT case above), so the case runs without re-running HOS-NWT.

### `tutorials/HOS-NWT-Irregular/` + `tutorials/waveFoam-HOS-Irregular/`

The same small flume driven by **irregular waves** (JONSWAP spectrum,
Hs = 0.05 m, Tp = 1 s).  Generate the HOS data:

```bash
cd HOSwaves2Foam/tutorials/HOS-NWT-Irregular
bash runHOS-NWT.sh
cp Results/modes_HOS_SWENSE.dat ../waveFoam-HOS-Irregular/constant/
```

Then run the CFD case (the irregular mode file is already included):

```bash
cd HOSwaves2Foam/tutorials/waveFoam-HOS-Irregular
bash Allrun
```

### `tutorials/stokesI-HOS/`

A large 624 m x 200 m wave tank (z-vertical, single cell in y) converted
from the original **Stokes-I** (IHFoam `waveModel StokesI`, H = 0.05 m,
T = 3 s) setup to **HOS waves**.  Its HOS-NWT generator case is in the
`HOS-NWT/` sub-directory (200 m tank, 100 m depth, amplitude 0.025 m,
frequency 1/3 Hz); the SWENSE mode file is already included:

```bash
cd HOSwaves2Foam/tutorials/stokesI-HOS
bash ./Allrun
```

The base 156x1x33 mesh is coarse on purpose: uncomment the four
`topoSet`/`refineMesh` passes in `Allrun` for production runs that resolve
the free surface.

### Pitfalls found while setting up the tutorial

* **`zMax` must be positive** in `Grid2Grid.dict` (the z-grid must extend
  above the free surface into the air); `zMin` below the water is negative.
* **2D (n2=1) HOS cases only provide the tank centreline y = 0** — the CFD
  mesh must have a single cell in y centred on y = 0.
* **`referenceTime`**: the official `Grid2Grid` class corrects Grid2Grid with
  the raw OpenFOAM time in `step()` (without `referenceTime`), while `eta`/
  `U` are queried with `time + referenceTime`.  Keeping `referenceTime = 0`
  keeps both consistent.
* **Coverage**: the HOS-NWT simulation must run long enough for the waves to
  reach the coupling zone (wave celerity x time), and the OpenFOAM case must
  be long enough for the waves to enter the domain.
* For the shallow (h = 0.4 m) tutorial tank the **hinged wavemaker was
  unstable**; the piston wavemaker (`igeom 1`) is used instead.
* HOS-NWT build notes (gfortran 9 / modern toolchains): drop the
  `-fno-automatic` flag (breaks ELEMENTAL/PURE procedures) and guard the
  `MOD(order-1, p-1)` calls in `sources/main/dealiasing.f90` against a zero
  divisor (`p2 = 1` is a compile-time constant).  Build single-threaded to
  avoid gfortran `.mod0` rename races, and point the LAPACK/BLAS discovery
  at a directory containing `liblapack.a`/`libblas.a`.

## References

* Yuan & Zhuang, "Regular and Irregular Wave Generation in OpenFOAM Using
  High Order Spectral Method", 13th OpenFOAM Workshop, 2018.
* "Generation of Regular and Irregular Waves in Navier-Stokes CFD Solvers by
  Matching With the Nonlinear Potential Wave Solution at the Boundaries",
  OMAE2018.
* "Parametric study of a new HOS-CFD coupling method in OpenFOAM",
  15th OpenFOAM Workshop, 2020.
* Grid2Grid / HOS-NWT / HOS-ocean: LHEEA, Ecole Centrale de Nantes
  (<https://gitlab.com/lheea/Grid2Grid>, <https://gitlab.com/lheea/HOS-NWT>).
