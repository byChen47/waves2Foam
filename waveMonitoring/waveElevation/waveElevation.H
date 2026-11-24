/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2026 byChen47
-------------------------------------------------------------------------------
License
    This file is part of waves2Foam (waveMonitoring module).

    waves2Foam is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    waves2Foam is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with waves2Foam.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::functionObjects::waveElevation

Group
    grpFieldFunctionObjects

Description
    Free-surface elevation time series at a list of gauge locations, for any
    two-phase (VOF) solver - no waves2Foam solver or library is required.

    The elevation at a gauge is the *integrated liquid height* along a ray
    through the gauge, in the direction opposite to gravity (or to the user
    supplied \c direction):

        eta = integral( alpha dl )

    which, for a flat-bottom tank, is exactly the free-surface elevation above
    the bottom. Using the alpha integral (rather than a single iso-surface
    crossing) keeps the reading well behaved for aerated / broken surfaces.

    One accumulating ASCII file is written per function object:

        postProcessing/waveElevation/<name>/elevation.dat

    with one row per write time: \c time  eta_0  eta_1  ... (tab separated),
    preceded by \c '#' comment lines that record the gauge locations, so the
    file is self-describing and can be read by \c waveSpectra.

Usage
    Example of function object specification:
    \verbatim
    waveElevation1
    {
        type            waveElevation;
        libs            ("libwaveMonitoring.so");

        // Mandatory
        locations       ((0 0 0.1) (1.0 0 0.1) (2.0 0 0.1));

        // Optional
        alpha           alpha.water;   // default: alpha.water
        liquid          true;          // default: true (alpha = liquid)
        direction       (0 0 0);       // default: (0 0 0) -> use g
        interpolationScheme cellPoint; // default: cellPoint
        subtractReference false;       // default: false (absolute elevation)
        log             true;          // default: true

        // Inherited (writeControl controls the sampling rate)
        writeControl    timeStep;
        writeInterval   1;
    }
    \endverbatim

    where the entries mean:
    \table
      Property     | Description                                  | Type | Reqd | Deflt
      type         | Type name: waveElevation                     | word | yes  | -
      libs         | Library name: libwaveMonitoring              | word | yes  | -
      locations    | Gauge locations                              | vectorList | yes | -
      alpha        | Name of the VOF field                        | word | no   | alpha.water
      liquid       | Flag if alpha is the liquid phase            | bool | no   | true
      direction    | Direction of the (downward) interface normal | vector | no | (0 0 0)
      interpolationScheme | Interpolation scheme for alpha        | word | no   | cellPoint
      subtractReference | Subtract eta at the first write         | bool | no   | false
      log          | Report the gauges in the log                 | bool | no   | true
    \endtable

    Output file: \c postProcessing/waveElevation/<name>/elevation.dat

Note
    - Works with any solver that has an \c alpha field and a \c g field.
    - The gauge rays must be inside the mesh; the integration runs from the
      mesh boundary through the gauge location.
    - Parallel: the integral is reduced over all processors, output is on the
      master only.

SourceFiles
    waveElevation.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_functionObjects_waveElevation_H
#define Foam_functionObjects_waveElevation_H

#include "fvMeshFunctionObject.H"
#include "point.H"
#include "OFstream.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{

/*---------------------------------------------------------------------------*\
                         Class waveElevation Declaration
\*---------------------------------------------------------------------------*/

class waveElevation
:
    public fvMeshFunctionObject
{
    // Private Data

        //- Is the alpha field that of the liquid phase?
        bool liquid_;

        //- Name of the alpha field
        word alphaName_;

        //- Interpolation scheme for the alpha field
        word interpolationScheme_;

        //- Direction of the ray (zero -> opposite to gravity)
        vector direction_;

        //- Gauge locations
        List<point> locations_;

        //- Subtract the elevation at the first write
        bool subtractReference_;

        //- Reference elevations (captured on the first write)
        scalarField reference_;

        //- Have the reference elevations been captured?
        bool haveReference_;

        //- Output stream
        autoPtr<OFstream> filePtr_;


    // Private Member Functions

        //- No copy construct
        waveElevation(const waveElevation&) = delete;

        //- No copy assignment
        void operator=(const waveElevation&) = delete;

        //- Write the file header
        void writeHeader();

        //- Open the output stream
        void openFile();

        //- Elevation at the given gauge location (parallel safe, master valid)
        scalar elevation(const point& loc, const vector& gHat) const;


public:

    //- Runtime type information
    TypeName("waveElevation");


    // Constructors

        //- Construct from Time and dictionary
        waveElevation
        (
            const word& name,
            const Time& runTime,
            const dictionary& dict
        );


    //- Destructor
    virtual ~waveElevation() = default;


    // Member Functions

        //- Read the function-object dictionary
        virtual bool read(const dictionary& dict);

        //- Execute (no-op, sampling happens in write)
        virtual bool execute();

        //- Write the elevation time series
        virtual bool write();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace functionObjects
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
