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
    waveSpectra

Description
    Spectral and zero-crossing analysis of the elevation time series written
    by the waveMonitoring function object (waveElevation).

    For every gauge column in the input file it reports

      Hm0, Tp, Tm02, Tm01   - spectral wave height / periods
      H1/3, T1/3, Hmax      - zero-crossing (time-domain) statistics
      mwl                   - mean water level (record mean)

    and writes one spectrum file per gauge plus a summary table.

Usage
    waveSpectra [-file <elevation.dat>] [-out <dir>]
                [-tStart <t>] [-tEnd <t>] [-df <df>] [-fMax <f>]
                [-gauge <i>|all]

    Defaults
        -file    postProcessing/waveElevation/waveElevation1/elevation.dat
                 (if not given, the first elevation.dat found under
                  postProcessing/waveElevation is used)
        -out     waveSpectra (relative to the current directory)
        -tStart  0          (first sample)
        -tEnd    end of record
        -df      1/T        (record length)
        -fMax    1/(2 dt)   (Nyquist)
        -gauge   all

\*---------------------------------------------------------------------------*/

#include "OFstream.H"
#include "OSspecific.H"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace
{

const double PI = 3.14159265358979323846;

struct Series
{
    std::vector<double> t;
    std::vector<double> eta;
};

struct Stats
{
    double mwl = 0;
    double m0 = 0;
    double hm0 = 0;
    double tp = 0;
    double tm02 = 0;
    double tm01 = 0;
    double h13 = 0;
    double t13 = 0;
    double hmax = 0;
    double hmean = 0;
    int nWaves = 0;
};

// Read the elevation file (columns: time eta0 eta1 ...)
bool readSeries
(
    const std::string& path,
    std::vector<double>& times,
    std::vector<std::vector<double> >& gauges,
    std::vector<std::string>& names
)
{
    std::ifstream is(path.c_str());

    if (!is.good())
    {
        return false;
    }

    std::string line;
    bool haveHeader = false;

    while (std::getline(is, line))
    {
        if (line.empty())
        {
            continue;
        }

        if (line[0] == '#')
        {
            // "# time<TAB>eta0<TAB>eta1 ..."
            std::istringstream hs(line.substr(1));
            std::string tok;

            hs >> tok;

            if (tok == "time")
            {
                names.clear();

                while (hs >> tok)
                {
                    names.push_back(tok);
                }

                haveHeader = !names.empty();
            }

            continue;
        }

        std::istringstream ds(line);
        double t;
        std::vector<double> row;

        if (!(ds >> t))
        {
            continue;
        }

        double v;
        while (ds >> v)
        {
            row.push_back(v);
        }

        if (row.empty())
        {
            continue;
        }

        times.push_back(t);
        gauges.push_back(row);
    }

    if (!haveHeader && !times.empty())
    {
        names.clear();

        for (size_t i = 0; i < gauges.front().size(); ++i)
        {
            names.push_back("eta" + std::to_string(i));
        }
    }

    return !times.empty();
}

// Non-uniform-safe DFT-based spectrum (amplitude at f from a least-squares
// projection onto cos/sin), one-sided variance density
void spectrum
(
    const Series& s,
    double df,
    double fMax,
    std::vector<double>& f,
    std::vector<double>& S
)
{
    f.clear();
    S.clear();

    const size_t n = s.t.size();

    if (n < 4)
    {
        return;
    }

    // Mean removal
    double mean = 0;
    for (size_t i = 0; i < n; ++i)
    {
        mean += s.eta[i];
    }
    mean /= double(n);

    // Record length
    const double T = s.t.back() - s.t.front();

    if (T <= 0)
    {
        return;
    }

    const int nf = std::max(1, int(std::floor(fMax/df)));

    for (int k = 1; k <= nf; ++k)
    {
        const double fk = k*df;

        // Skip the Nyquist bin: cos/sin are degenerate there and the
        // one-sided density would be double counted
        if (fk >= fMax)
        {
            continue;
        }

        double sumXc = 0, sumXs = 0;

        for (size_t i = 0; i < n; ++i)
        {
            const double ph = 2.0*PI*fk*s.t[i];
            const double x = s.eta[i] - mean;

            sumXc += x*std::cos(ph);
            sumXs += x*std::sin(ph);
        }

        // Projection onto cos/sin (exact for a uniform grid whose record
        // length is a multiple of 1/f; robust, unlike the 2x2 normal
        // equations which become singular near Nyquist)
        const double a = 2.0*sumXc/double(n);
        const double b = 2.0*sumXs/double(n);

        // One-sided variance density: A^2/(2 df), A^2 = a^2 + b^2
        const double A2 = a*a + b*b;

        f.push_back(fk);
        S.push_back(A2/(2.0*df));
    }
}

Stats analyse(const Series& s, double df, double fMax, std::vector<double>& f, std::vector<double>& S)
{
    Stats st;

    const size_t n = s.t.size();

    double mean = 0;
    for (size_t i = 0; i < n; ++i)
    {
        mean += s.eta[i];
    }
    mean /= double(n);
    st.mwl = mean;

    // ---- spectral ----
    spectrum(s, df, fMax, f, S);

    double m0 = 0, m1 = 0, m2 = 0;
    double sMax = -1, fPeak = 0;

    for (size_t k = 0; k < f.size(); ++k)
    {
        const double Sk = S[k];
        m0 += Sk*df;
        m1 += f[k]*Sk*df;
        m2 += f[k]*f[k]*Sk*df;

        if (Sk > sMax)
        {
            sMax = Sk;
            fPeak = f[k];
        }
    }

    st.m0 = m0;
    st.hm0 = 4.0*std::sqrt(std::max(0.0, m0));
    st.tp = (fPeak > 0) ? 1.0/fPeak : 0;
    st.tm02 = (m2 > 0) ? std::sqrt(std::max(0.0, m0/m2)) : 0;
    st.tm01 = (m1 > 0) ? m0/m1 : 0;

    // ---- zero crossings (up-crossings of the de-meaned signal) ----
    std::vector<double> tc;   // up-crossing times
    std::vector<double> hh;   // wave heights

    for (size_t i = 1; i < n; ++i)
    {
        const double a0 = s.eta[i-1] - mean;
        const double a1 = s.eta[i]   - mean;

        if (a0 <= 0 && a1 > 0)
        {
            const double w = -a0/(a1 - a0);
            tc.push_back(s.t[i-1] + w*(s.t[i] - s.t[i-1]));
        }
    }

    for (size_t c = 0; c + 1 < tc.size(); ++c)
    {
        double hi = -1e300, lo = 1e300;

        for (size_t i = 0; i < n; ++i)
        {
            if (s.t[i] >= tc[c] && s.t[i] <= tc[c+1])
            {
                hi = std::max(hi, s.eta[i]);
                lo = std::min(lo, s.eta[i]);
            }
        }

        if (hi > -1e299 && lo < 1e299)
        {
            hh.push_back(hi - lo);
        }
    }

    st.nWaves = int(hh.size());

    if (!hh.empty())
    {
        double hsum = 0, hmax = 0;

        for (double h : hh)
        {
            hsum += h;
            hmax = std::max(hmax, h);
        }

        st.hmean = hsum/double(hh.size());
        st.hmax = hmax;

        // H1/3, T1/3: mean of the highest third
        std::vector<size_t> idx(hh.size());
        for (size_t i = 0; i < idx.size(); ++i)
        {
            idx[i] = i;
        }

        std::sort
        (
            idx.begin(), idx.end(),
            [&](size_t a, size_t b) { return hh[a] > hh[b]; }
        );

        const size_t n13 = std::max<size_t>(1, hh.size()/3);

        double h13 = 0, t13 = 0;

        for (size_t i = 0; i < n13; ++i)
        {
            h13 += hh[idx[i]];
            t13 += (tc[idx[i]+1] - tc[idx[i]]);
        }

        st.h13 = h13/double(n13);
        st.t13 = t13/double(n13);
    }

    return st;
}

void printUsage()
{
    std::cout <<
        "Usage: waveSpectra [-file <elevation.dat>] [-out <dir>]\n"
        "                   [-tStart <t>] [-tEnd <t>] [-df <df>] [-fMax <f>]\n"
        "                   [-gauge <i>|all]\n";
}

} // End anonymous namespace


int main(int argc, char* argv[])
{
    std::string file, outDir = "waveSpectra";
    double tStart = -1e300, tEnd = 1e300, df = 0, fMax = 0;
    std::string gaugeSel = "all";

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

        if (opt == "-file")       file = next("-file");
        else if (opt == "-out")   outDir = next("-out");
        else if (opt == "-tStart") tStart = std::atof(next("-tStart").c_str());
        else if (opt == "-tEnd")   tEnd = std::atof(next("-tEnd").c_str());
        else if (opt == "-df")     df = std::atof(next("-df").c_str());
        else if (opt == "-fMax")   fMax = std::atof(next("-fMax").c_str());
        else if (opt == "-gauge")  gaugeSel = next("-gauge");
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

    std::vector<double> times;
    std::vector<std::vector<double> > gauges;
    std::vector<std::string> names;

    if (file.empty())
    {
        file = "postProcessing/waveElevation/waveElevation1/elevation.dat";
    }

    if (!readSeries(file, times, gauges, names))
    {
        std::cerr << "Cannot read elevation data from: " << file << "\n"
                  << "Use -file <path> to point at an elevation.dat file."
                  << std::endl;
        return 1;
    }

    std::cout << "waveSpectra: read " << times.size() << " samples, "
              << names.size() << " gauge(s) from " << file << std::endl;

    // Time window
    std::vector<size_t> sel;

    for (size_t i = 0; i < times.size(); ++i)
    {
        if (times[i] >= tStart && times[i] <= tEnd)
        {
            sel.push_back(i);
        }
    }

    if (sel.size() < 8)
    {
        std::cerr << "Not enough samples in the selected time window ("
                  << sel.size() << ")" << std::endl;
        return 1;
    }

    Foam::mkDir(outDir);

    const double T = times[sel.back()] - times[sel.front()];
    const double dt = T/double(std::max<size_t>(1, sel.size() - 1));

    if (df <= 0)
    {
        df = 1.0/T;
    }

    if (fMax <= 0)
    {
        fMax = 0.5/dt;
    }

    std::cout << "  window: t = " << times[sel.front()] << " .. "
              << times[sel.back()] << "  (" << sel.size() << " samples, dt = "
              << dt << " s)" << std::endl
              << "  df = " << df << " 1/s   fMax = " << fMax << " 1/s" << std::endl
              << "  out: " << outDir << std::endl;

    std::cout << "\n"
              << "  gauge      Hm0      Tp     Tm02     Tm01     H1/3    T1/3"
              << "      Hmax    Hmean     mwl   waves" << std::endl;

    Foam::OFstream stats(outDir + "/waveStats.dat");

    stats << "# waveSpectra summary\n"
          << "# file: " << file << "\n"
          << "# tStart: " << times[sel.front()] << "  tEnd: " << times[sel.back()]
          << "  n: " << sel.size() << "  dt: " << dt << "\n"
          << "# df: " << df << "  fMax: " << fMax << "\n"
          << "# gauge\tHm0\tTp\tTm02\tTm01\tH1/3\tT1/3\tHmax\tHmean\tmwl\tnWaves\n";

    for (size_t g = 0; g < gauges.front().size(); ++g)
    {
        if (gaugeSel != "all" && std::atoi(gaugeSel.c_str()) != int(g))
        {
            continue;
        }

        Series s;

        for (size_t k : sel)
        {
            if (g < gauges[k].size())
            {
                s.t.push_back(times[k]);
                s.eta.push_back(gauges[k][g]);
            }
        }

        std::vector<double> f, S;
        const Stats st = analyse(s, df, fMax, f, S);

        const std::string gname =
            (g < names.size()) ? names[g] : ("eta" + std::to_string(g));

        // Spectrum file
        Foam::OFstream os(outDir + "/" + gname + "_spectrum.dat");

        os  << "# waveSpectra: " << gname.c_str() << "\n"
            << "# file: " << file << "\n"
            << "# Hm0: " << st.hm0 << "  Tp: " << st.tp
            << "  Tm02: " << st.tm02 << "  m0: " << st.m0 << "\n"
            << "# frequency[S(f)] variance density\n";

        for (size_t k = 0; k < f.size(); ++k)
        {
            os  << f[k] << ' ' << S[k] << '\n';
        }

        stats << gname.c_str() << '\t'
              << st.hm0 << '\t' << st.tp << '\t' << st.tm02 << '\t'
              << st.tm01 << '\t' << st.h13 << '\t' << st.t13 << '\t'
              << st.hmax << '\t' << st.hmean << '\t' << st.mwl << '\t'
              << st.nWaves << '\n';

        std::cout << "  " << gname << '\t'
                  << st.hm0 << '\t' << st.tp << '\t' << st.tm02 << '\t'
                  << st.tm01 << '\t' << st.h13 << '\t' << st.t13 << '\t'
                  << st.hmax << '\t' << st.hmean << '\t' << st.mwl << '\t'
                  << st.nWaves << std::endl;
    }

    std::cout << "\nWrote " << outDir << "/waveStats.dat and "
              << outDir << "/<gauge>_spectrum.dat" << std::endl;

    return 0;
}


// ************************************************************************* //
