/* ----------------------------------------------------------------------
 *
 *                    ***       Karamelo       ***
 *               Parallel Material Point Method Simulator
 *
 * Copyright (2019) Alban de Vaucorbeil, alban.devaucorbeil@monash.edu
 * Materials Science and Engineering, Monash University
 * Clayton VIC 3800, Australia

 * This software is distributed under the GNU General Public License.
 *
 * ----------------------------------------------------------------------- */

#include "fix_indent_hertz.h"
#include "domain.h"
#include "error.h"
#include "group.h"
#include "input.h"
#include "solid.h"
#include "universe.h"
#include "update.h"
#include <Eigen/Eigen>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace FixConst;
using namespace Eigen;

#define four_thirds 1.333333333

FixIndentHertz::FixIndentHertz(MPM *mpm, vector<string> args)
    : Fix(mpm, args) {
  if (args.size() < Nargs) {
    error->all(FLERR, "Error: not enough arguments.\n" + usage);
  }

  type_pos = 3;
  Rpos = 4;
  xpos = 5;
  ypos = 6;
  zpos = 7;
  vxpos = 8;
  vypos = 9;
  vzpos = 10;

  if (group->pon[igroup].compare("particles") != 0 &&
      group->pon[igroup].compare("all") != 0) {
    error->all(FLERR, "fix_indent_hertz needs to be given a group of nodes" +
                          group->pon[igroup] + ", " + args[2] +
                          " is a group of " + group->pon[igroup] + ".\n");
  }
  cout << "Creating new fix FixIndentHertz with ID: " << args[0] << endl;
  id = args[0];

  type = args[type_pos];
  if (args[type_pos].compare("sphere") == 0) {
    type = "sphere";
  } else {
    error->all(FLERR, "Error indent type " + args[type_pos] +
                          " unknown. Only type sphere is supported.\n");
  }
}

FixIndentHertz::~FixIndentHertz() {}

void FixIndentHertz::init() {}

void FixIndentHertz::setup() {}

void FixIndentHertz::setmask() {
  mask = 0;
  mask |= INITIAL_INTEGRATE;
}

void FixIndentHertz::initial_integrate() {
  // cout << "In FixIndentHertz::initial_integrate()\n";

  // Go through all the particles in the group and set b to the right value:
  Eigen::Vector3d f;

  int solid = group->solid[igroup];

  Solid *s;
  Eigen::Vector3d ftot, ftot_reduced;

  double R = input->parsev(args[Rpos]).result(mpm);
  Eigen::Vector3d xs(input->parsev(args[xpos]).result(mpm),
                     input->parsev(args[ypos]).result(mpm),
                     input->parsev(args[zpos]).result(mpm));
  Eigen::Vector3d vs(input->parsev(args[vxpos]).result(mpm),
                     input->parsev(args[vypos]).result(mpm),
                     input->parsev(args[vzpos]).result(mpm));
  Eigen::Vector3d xsp;

  double Rs, Rp, r, p, pdot, f1, f2, fmag;

  ftot.setZero();

  if (solid == -1) {
    for (int isolid = 0; isolid < domain->solids.size(); isolid++) {
      s = domain->solids[isolid];

      if (domain->dimension == 2) {
        for (int ip = 0; ip < s->np_local; ip++) {
          if (s->mass[ip] > 0) {
            if (s->mask[ip] & groupbit) {

              // Gross screening:
              xsp = s->x[ip] - xs;


              Rp = 0.5*sqrt(s->vol[ip]);
              Rs = R + Rp;

	      r = xsp.norm();
	      

              if ((xsp[0] < Rs) && (xsp[1] < Rs) && (xsp[0] > -Rs) &&
                  (xsp[1] > -Rs)) {

                r = xsp.norm();
                // Finer screening:
                if (r < Rs) {
                  p = Rs - r; // penetration

                  if (p > 0) {
                    fmag = 0.25 * M_PI * s->mat->E / (1 - s->mat->nu * s->mat->nu) * p;

                    // fmag = K * MIN(f1, f2);

                    f = fmag * xsp / r;
                    s->mbp[ip] += f;
                    ftot += f;
                  } else {
                    fmag = 0;
                    f.setZero();
                  }
                }
              }
            }
          }
        }
      }

      if (domain->dimension == 3) {
        for (int ip = 0; ip < s->np_local; ip++) {
          if (s->mass[ip] > 0) {
            if (s->mask[ip] & groupbit) {

              // Gross screening:
              xsp = s->x[ip] - xs;

              Rp = 0.5*pow(s->vol[ip], 0.333333333);
              Rs = R + Rp;

              if ((xsp[0] < Rs) && (xsp[1] < Rs) && (xsp[2] < Rs) &&
                  (xsp[0] > -Rs) && (xsp[1] > -Rs) && (xsp[2] > -Rs)) {

                r = xsp.norm();
                // Finer screening:
                if (r < Rs) {
                  p = Rs - r; // penetration

                  if (p > 1.0e-10) {
                    fmag = four_thirds * s->mat->E / (1 - s->mat->nu * s->mat->nu) * sqrt(R * p * p * p);

                    // fmag = K * MIN(f1, f2);

                    f = fmag * xsp / r;
                    s->mbp[ip] += f;
                    ftot += f;
                  } else {
                    fmag = 0;
                    f.setZero();
                  }
                }
              }
            }
          }
        }
      }
    }
  } else {
    s = domain->solids[solid];

    if (domain->dimension == 2) {
      for (int ip = 0; ip < s->np_local; ip++) {
        if (s->mass[ip] > 0) {
          if (s->mask[ip] & groupbit) {

            // Gross screening:
            xsp = s->x[ip] - xs;

            Rp = 0.5*sqrt(s->vol0[ip]);
            Rs = R + Rp;

            if ((xsp[0] < Rs) && (xsp[1] < Rs) && (xsp[0] > -Rs) &&
                (xsp[1] > -Rs)) {

              r = xsp.norm();
              // Finer screening:
              if (r < Rs) {
                p = Rs - r; // penetration

                if (p > 0) {
		  fmag = 0.25 * M_PI * s->mat->E / (1 - s->mat->nu * s->mat->nu) * p;

                  // fmag = K * MIN(f1, f2);

                  f = fmag * xsp / r;
                  s->mbp[ip] += f;
                  ftot += f;
                } else {
                  fmag = 0;
                  f.setZero();
                }
              }
            }
          }
        }
      }
    }
    if (domain->dimension == 3) {
      for (int ip = 0; ip < s->np_local; ip++) {
        if (s->mass[ip] > 0) {
          if (s->mask[ip] & groupbit) {

            // Gross screening:
            xsp = s->x[ip] - xs;

            Rp = 0.5*pow(s->vol[ip], 0.333333333);
            Rs = R + Rp;

            if ((xsp[0] < Rs) && (xsp[1] < Rs) && (xsp[2] < Rs) &&
                (xsp[0] > -Rs) && (xsp[1] > -Rs) && (xsp[2] > -Rs)) {

              r = xsp.norm();
              // Finer screening:
              if (r < Rs) {
                p = Rs - r; // penetration

                if (p > 0) {
		  fmag = four_thirds * s->mat->E / (1 - s->mat->nu * s->mat->nu) * sqrt(R * p * p * p);
                  // fmag = K * MIN(f1, f2);

                  f = fmag * xsp / r;
                  s->mbp[ip] += f;
                  ftot += f;
                } else {
                  fmag = 0;
                  f.setZero();
                }
              }
            }
          }
        }
      }
    }
  }

  // Reduce ftot:
  MPI_Allreduce(ftot.data(), ftot_reduced.data(), 3, MPI_DOUBLE, MPI_SUM,
                universe->uworld);

  (*input->vars)[id + "_x"] = Var(id + "_x", ftot_reduced[0]);
  (*input->vars)[id + "_y"] = Var(id + "_y", ftot_reduced[1]);
  (*input->vars)[id + "_z"] = Var(id + "_z", ftot_reduced[2]);
}
