//******************************************
// operators.f90
// based on min-app code written by Oliver Fuhrer, MeteoSwiss
// modified by Ben Cumming, CSCS
// *****************************************

// Description: Contains simple operators which can be used on 3d-meshes

#include "data.h"
#include "operators.h"
#include "stats.h"

namespace operators {

// input: s, gives updated solution for f
// only handles interior grid points, as boundary points are fixed
// those inner grid points neighbouring a boundary point, will in the following
// be referred to as boundary points and only those grid points
// only neighbouring non-boundary points are called inner grid points
void diffusion(const data::Field &s, data::Field &f)
{
    using data::options;

    using data::bndE;
    using data::bndW;
    using data::bndN;
    using data::bndS;

    using data::y_old;

    double alpha = options.alpha;
    double beta = options.beta;

    int nx = options.nx;
    int iend  = nx - 1;
    int jend  = nx - 1;

    // the interior grid points
    #pragma omp parallel for
    for (int j=1; j < jend; j++) {
        for (int i=1; i < iend; i++) {
            f(i,j) = -(4. + alpha) * s(i,j)
                        + s(i+1,j) + s(i-1,j) + s(i,j-1) + s(i,j+1)
                        + beta * s(i,j) * (1.0 - s(i,j))
                        + alpha * y_old(i,j);

        }
    }

    // the east boundary
    #pragma omp sections nowait
    {
        #pragma omp section 
        {
            int i = nx - 1;
            for (int j = 1; j < jend; j++) {
                f(i,j) = -(4. + alpha) * s(i,j)
                            + s(i-1,j) + s(i,j-1) + s(i,j+1)
                            + alpha * y_old(i,j) + bndE[j]
                            + beta * s(i,j) * (1.0 - s(i,j));
            }
        }

        // the west boundary
        #pragma omp section
        {
            int i = 0;
            for(int j = 1; j < jend; j++) {
                f(i,j) = -(4. + alpha) * s(i,j)
                    + s(i+1,j) + s(i,j-1) + s(i,j+1)
                    + beta * s(i,j) * (1.0 - s(i,j))
                    + alpha * y_old(i,j) + bndW[j];
            }
        }

        // the north boundary (plus NE and NW corners)
        #pragma omp section
        {
            int j = jend;

            {
                int i = 0; // NW corner
                f(i,j) = -(4. + alpha) * s(i,j)
                            + s(i+1,j) + s(i,j-1)
                            + alpha * y_old(i,j) + bndW[j] + bndN[i]
                            + beta * s(i,j) * (1.0 - s(i,j));
            }

            // inner north boundary
            //#pragma omp parallel shared(j)
            for(int i = 1; i < iend; i++) {
                f(i,j) = -(4. + alpha) * s(i,j)
                            + s(i+1,j) + s(i-1,j) + s(i,j-1)
                            + beta * s(i,j) * (1.0 - s(i,j))
                            + alpha * y_old(i,j) + bndN[i];
            }

            {
                int i = iend; // NE corner
                f(i,j) = -(4. + alpha) * s(i,j)
                            + s(i-1,j) + s(i,j-1)
                            + alpha * y_old(i,j) + bndE[j] + bndN[i]
                            + beta * s(i,j) * (1.0 - s(i,j));
            }
        }

        // the south boundary
        #pragma omp section
        {
            int j = 0;

            {
                int i = 0; // SW corner
                f(i,j) = -(4. + alpha) * s(i,j)
                            + s(i+1,j) + s(i,j+1)
                            + alpha * y_old(i,j) + bndW[j] + bndS[i]
                            + beta * s(i,j) * (1.0 - s(i,j));
            }

            // inner south boundary
            for(int i = 1; i < iend; i++) {
                f(i,j) = -(4. + alpha) * s(i,j)
                            + s(i+1,j) + s(i-1,j) + s(i,j+1)
                            + beta * s(i,j) * (1.0 - s(i,j))
                            + alpha * y_old(i,j) + bndS[i];
            }

            {
                int i = iend; // SE corner
                f(i,j) = -(4. + alpha) * s(i,j)
                            + s(i-1,j) + s(i,j+1)
                            + alpha * y_old(i,j) + bndE[j] + bndS[i]
                            + beta * s(i,j) * (1.0 - s(i,j));
            }
        }
    }

    // Accumulate the flop counts
    // 8 ops total per point
    stats::flops_diff +=
        + 12 * (nx - 2) * (nx - 2) // interior points
        + 11 * (nx - 2  +  nx - 2) // NESW boundary points
        + 11 * 4;                                  // corner points
}

} // namespace operators
