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

Application
    GaugesNProbes

Description
    Standalone replacement for waves2Foam's \c waveGaugesNProbes: reads the
    gauge/probe layout from \c constant/probeDefinitions and writes a ready to
    use \c functions fragment for \c system/controlDict.

    For every dictionary entry in \c probeDefinitions:

    * \c type \c waveGauge  -> one \c waveElevation function object with the
      generated gauge locations (\c waveMonitoring module);
    * \c type \c probeGauge -> one OpenFOAM \c probes function object with the
      generated probe locations.

    The generated file is meant to be included inside the \c functions block:

    \verbatim
    functions
    {
        #include "gaugesNProbes.functions"
    }
    \endverbatim

    The layout keywords are the waves2Foam ones, so existing
    \c probeDefinitions files work unchanged:

    \verbatim
    <name>
    {
        type                waveGauge;

        pointDistribution   lineDistribution;
        N                   179;
        linestart           (0.10 -0.3 0.05);
        lineend             (17.9 -0.3 0.05);
        add                 (0 0.45 0);     // start -> end of the gauge line
        axis                y;
        stretch             1.0;

        // optional, written into the generated function object
        writeControl        adjustableRunTime;
        writeInterval       0.025;
        alpha               alpha.water;
    }
    \endverbatim

    Distribution types: \c lineDistribution, \c circularDistribution,
    \c quadrilateralDistribution, \c concatenatedDistribution,
    \c userDefinedDistribution.

Usage
    GaugesNProbes [-case <dir>] [-dict <file>] [-out <file>] [-noVTK]
                  [-functionObject <type>] [-libs <lib>]

    Defaults
        -case            .
        -dict            constant/probeDefinitions
        -out             system/gaugesNProbes.functions
        -functionObject  waveElevation
        -libs            libwaveMonitoring.so

    Also writes \c gaugesNProbes/<name>.vtk (unless \c -noVTK) so the gauge
    layout can be checked in ParaView.

\*---------------------------------------------------------------------------*/

#include "IFstream.H"
#include "OFstream.H"
#include "OSspecific.H"
#include "pointField.H"
#include "dictionary.H"

#include "pointDistributions.H"

#include <iostream>
#include <string>

using namespace Foam;

// * * * * * * * * * * * * * * * Local Helpers  * * * * * * * * * * * * * * //

namespace
{

void printUsage()
{
    std::cout <<
        "Usage: GaugesNProbes [-case <dir>] [-dict <file>] [-out <file>]\n"
        "                     [-noVTK] [-functionObject <type>] [-libs <lib>]\n"
        "\n"
        "  -case            case directory                 (default: .)\n"
        "  -dict            gauge layout dictionary        (default: constant/probeDefinitions)\n"
        "  -out             generated functions fragment   (default: system/gaugesNProbes.functions)\n"
        "  -functionObject  function object type for waveGauge entries (default: waveElevation)\n"
        "  -libs            library name for the function object (default: libwaveMonitoring.so)\n"
        "  -noVTK           skip the gaugesNProbes/<name>.vtk visualisation\n";
}


// Write gauge lines (waveGauge) or points (probeGauge) as legacy VTK
void writeVtk
(
    const fileName& path,
    const pointField& ppS,
    const pointField& ppE,
    const bool asLines
)
{
    OFstream os(path);

    os  << "# vtk DataFile Version 3.0" << nl
        << "GaugesNProbes" << nl
        << "ASCII" << nl
        << "DATASET POLYDATA" << nl;

    if (asLines)
    {
        os  << "POINTS " << 2*ppS.size() << " float" << nl;

        forAll(ppS, i)
        {
            os  << ppS[i].x() << ' ' << ppS[i].y() << ' ' << ppS[i].z() << nl;
        }

        forAll(ppE, i)
        {
            os  << ppE[i].x() << ' ' << ppE[i].y() << ' ' << ppE[i].z() << nl;
        }

        os  << "LINES " << ppS.size() << ' ' << 3*ppS.size() << nl;

        forAll(ppS, i)
        {
            os  << "2 " << i << ' ' << i + ppS.size() << nl;
        }
    }
    else
    {
        os  << "POINTS " << ppS.size() << " float" << nl;

        forAll(ppS, i)
        {
            os  << ppS[i].x() << ' ' << ppS[i].y() << ' ' << ppS[i].z() << nl;
        }

        os  << "VERTICES " << ppS.size() << ' ' << 2*ppS.size() << nl;

        forAll(ppS, i)
        {
            os  << "1 " << i << nl;
        }
    }
}


// Write a "locations ( ... )" block, wrapped at a fixed number of entries
void writeLocations
(
    Ostream& os,
    const pointField& pp
)
{
    os  << "        locations" << nl << "        (" << nl;

    forAll(pp, i)
    {
        os  << "            " << pp[i] << nl;
    }

    os  << "        );" << nl;
}


void writeProbeLocations(Ostream& os, const pointField& pp)
{
    os  << "        probeLocations" << nl << "        (" << nl;

    forAll(pp, i)
    {
        os  << "            " << pp[i] << nl;
    }

    os  << "        );" << nl;
}

} // End anonymous namespace


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char* argv[])
{
    fileName caseDir(".");
    fileName dictRel("constant/probeDefinitions");
    fileName outRel("system/gaugesNProbes.functions");
    word foType("waveElevation");
    word foLibs("libwaveMonitoring.so");
    bool writeVTK = true;

    for (int i = 1; i < argc; ++i)
    {
        const std::string opt(argv[i]);

        auto next = [&](const char* what) -> std::string
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Missing value for " << what << std::endl;
                printUsage();
                std::exit(1);
            }
            return std::string(argv[++i]);
        };

        if (opt == "-case")            caseDir = next("-case");
        else if (opt == "-dict")       dictRel = next("-dict");
        else if (opt == "-out")        outRel = next("-out");
        else if (opt == "-functionObject") foType = word(next("-functionObject"));
        else if (opt == "-libs")       foLibs = word(next("-libs"));
        else if (opt == "-noVTK")      writeVTK = false;
        else if (opt == "-help" || opt == "--help" || opt == "-h")
        {
            printUsage();
            return 0;
        }
        else
        {
            std::cerr << "Unknown option: " << opt << std::endl;
            printUsage();
            return 1;
        }
    }

    const fileName dictPath = caseDir/dictRel;

    IFstream is(dictPath);

    if (!is.good())
    {
        std::cerr << "Cannot read '" << dictPath << "'" << std::endl;
        return 1;
    }

    dictionary probeDefs(is);

    mkDir(caseDir/"gaugesNProbes");

    const fileName outPath = caseDir/outRel;

    OFstream os(outPath);

    os  << "/*--------------------------------*- C++ -*----------------------------------*\\\n"
        << "  Generated by GaugesNProbes from " << dictRel << "\n"
        << "  Include it inside the functions{} block of system/controlDict:\n"
        << "\n"
        << "      functions\n"
        << "      {\n"
        << "          #include \"" << outRel.name() << "\"\n"
        << "      }\n"
        << "\\*---------------------------------------------------------------------------*/\n\n";

    label nGroups = 0;
    label nGauges = 0;

    const wordList toc(probeDefs.toc());

    forAll(toc, item)
    {
        const word& name = toc[item];

        if (!probeDefs.isDict(name))
        {
            continue;
        }

        const dictionary& dict = probeDefs.subDict(name);

        const word type = dict.getOrDefault<word>("type", "waveGauge");

        autoPtr<pointDistributions> pd(pointDistributions::New(dict));

        const pointField ppS(pd->evaluateStart());

        // The end points are only used for the VTK visualisation of the gauge
        // lines; the individual distributions know how to build them when the
        // (waves2Foam) "add" offset is present
        pointField ppE(ppS);

        if (dict.found("add"))
        {
            ppE = pd->evaluateEnd();
        }

        if (ppS.empty())
        {
            std::cerr << "WARNING: '" << name << "' produced no points - skipped"
                      << std::endl;
            continue;
        }

        // Sampling controls (waves2Foam keywords, with sane defaults)
        const word writeControl =
            dict.getOrDefault<word>("writeControl", "timeStep");
        const scalar writeInterval =
            dict.getOrDefault<scalar>("writeInterval", 1.0);

        os  << "    " << name << nl << "    {" << nl;

        if (type == "waveGauge")
        {
            os  << "        type            " << foType << ';' << nl
                << "        libs            (\"" << foLibs << "\");" << nl << nl;

            writeLocations(os, ppS);

            os  << nl
                << "        alpha           "
                << dict.getOrDefault<word>("alpha", "alpha.water") << ';' << nl
                << "        writeControl    " << writeControl << ';' << nl
                << "        writeInterval   " << writeInterval << ';' << nl
                << "        log             false;" << nl;

            if (writeVTK)
            {
                writeVtk
                (
                    caseDir/"gaugesNProbes"/(name + ".vtk"),
                    ppS,
                    ppE,
                    true
                );
            }
        }
        else if (type == "probeGauge")
        {
            os  << "        type            probes;" << nl
                << "        libs            (sampling);" << nl << nl;

            writeProbeLocations(os, ppS);

            wordList fields(1, word("alpha.water"));

            if (dict.found("fields"))
            {
                fields = wordList(dict.lookup("fields"));
            }

            os  << nl << "        fields          (";

            forAll(fields, fi)
            {
                if (fi)
                {
                    os  << ' ';
                }
                os  << fields[fi];
            }

            os  << ");" << nl
                << "        writeControl    " << writeControl << ';' << nl
                << "        writeInterval   " << writeInterval << ';' << nl;

            if (writeVTK)
            {
                writeVtk
                (
                    caseDir/"gaugesNProbes"/(name + ".vtk"),
                    ppS,
                    ppE,
                    false
                );
            }
        }
        else
        {
            std::cerr << "WARNING: '" << name << "': unknown type '" << type
                      << "' - skipped" << std::endl;
            os  << "    }" << nl << nl;
            continue;
        }

        os  << "    }" << nl << nl;

        // Flush after every group, so that a bad entry further down (which
        // aborts with a FatalError) does not lose the groups already written
        os.flush();

        Info<< "  " << type << " '" << name << "': " << ppS.size()
            << " location(s), first = " << ppS[0] << endl;

        ++nGroups;
        nGauges += ppS.size();
    }

    Info<< nl << "Wrote " << outPath << " (" << nGroups << " function object(s), "
        << nGauges << " location(s))" << endl;

    if (writeVTK)
    {
        Info<< "Wrote " << caseDir/"gaugesNProbes" << "/<name>.vtk" << endl;
    }

    Info<< nl << "Add this to system/controlDict:" << nl << nl
        << "    functions" << nl
        << "    {" << nl
        << "        #include \"" << outRel.name() << "\"" << nl
        << "    }" << nl << endl;

    return 0;
}


// ************************************************************************* //
