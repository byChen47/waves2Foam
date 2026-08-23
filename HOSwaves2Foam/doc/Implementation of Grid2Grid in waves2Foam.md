# Implementation of Grid2Grid in waves2Foam

* Author: Young-Myung Choi, Bureau Veritas, Paris, France

* Date: 17/10/2020

* Document version: 1.0
* Mail to: youngmyung.choi@bureauveritas.com or benjamin.bouscasse@ec-nantes.fr

This document is the official integration guide from the Grid2Grid
repository (`example/waves2Foam/`) and is vendored here for reference.  The
code has been adapted for OpenFOAM v2206 in this folder (see the module
README); the installation steps below show the original (core-library)
integration, while this repository keeps the coupling self-contained in a
separate library instead of modifying the waves2Foam core.

## Remarks

As OpenFOAM has divergent version and the waves2Foam has many macros to support many OpenFOAM version, the provided example may not exactly fit into the version what user installed. Therefore, the user has a responsibility to install and implement the Grid2Grid Class. 

## How to install

The author assumes that the user's linux terminal is under the OpenFOAM environment and has a environmental variable **$WAVES_DIR** where it is defined in "waves2Foam/bin/bashrc".

* Compile the Grid2Grid (See cmake.org) 

  ```cmake
  cmake -H. -Bbuild -DBUILD_OF_LIB=ON
  cmake --build build
  ```

  Then, check the "libGrid2Grid.so" exists in $FOAM_USER_LIBBIN

  ```bash
  ls $FOAM_USER_LIBBIN/libGrid2Grid.so
  ```

1. Copy the folder "Grid2Grid" into waves2Foam library

   ```bash
   cp -r Grid2Grid $WAVES_DIR/src/waves2Foam/waveTheories/externalWaveForcing/
   ```

2. Copy the folder "Grid2GridProperties" into waves2FoamProcessing library

   ```bash
   cp -r Grid2GridProperties $WAVES_DIR/src/waves2FoamProcessing/preProcessing/setWaveProperties/externalWaveForcing
   ```

3. Change wmake settings of waves2Foam (add Grid2Grid.C to Make/files, add the
   Grid2Grid include path and `-lGrid2Grid` to Make/options).

4. Change wmake settings of waves2FoamProcessing (add Grid2GridProperties.C to
   Make/files).

5. Compile waves2Foam

   ```bash
   cd $WAVES_DIR
   ./Allwmake
   ```

6. Fix the bugs related to the OpenFOAM version.

## Caution

* The gravitational vector should be a negative z always. (Be careful when you generate the mesh). 

## Input Format (constant/waveProperties)

* For example

  ```c
  externalForcing     Grid2Grid;
  
  Grid2GridCoeffs
  {
      waveType            Grid2Grid;
  
      Tsoft               1;
  
      referenceTime       -20;
  
      referencePosition   (-10 -10 0);
  
      referenceTheta      0;
  
      Grid2GridInputFileName Grid2Grid.dict;
  }
  ```
