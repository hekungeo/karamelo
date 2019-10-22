#include <iostream>
#include "output.h"
#include "dump_particle.h"
#include "update.h"
#include "domain.h"
#include "solid.h"
#include "mpmtype.h"
#include "mpm_math.h"

using namespace std;
using namespace MPM_Math;


DumpParticle::DumpParticle(MPM *mpm, vector<string> args) : Dump(mpm, args)
{
  cout << "In DumpParticle::DumpParticle()" << endl;
  for (int i=5; i<args.size(); i++){
    if (find(known_var.begin(), known_var.end(), args[i]) != known_var.end()) {
      output_var.push_back(args[i]);
    } else {
      cout << "Error: output variable \033[1;31m" << args[i] << "\033[0m is unknown!\n";
      cout << "Availabe output variables: ";
      for (auto v: known_var) {
	cout << v << ", ";
      }
      cout << endl;
      exit(1);
    }
  }
}

DumpParticle::~DumpParticle()
{
}


void DumpParticle::write()
{
  // Open dump file:
  size_t pos_asterisk = filename.find('*');
  string fdump;

  if (pos_asterisk >= 0)
    {
      // Replace the asterisk by ntimestep:
      fdump = filename.substr(0, pos_asterisk)
	+ to_string(update->ntimestep);
      if (filename.size()-pos_asterisk-1 > 0)
	fdump += filename.substr(pos_asterisk+1, filename.size()-pos_asterisk-1);
    }
  else fdump = filename;

  // cout << "Filemame for dump: " << fdump << endl;

  // Open the file fdump:
  ofstream dumpstream(fdump, ios_base::out);

  if (dumpstream.is_open()) {
    dumpstream << "ITEM: TIMESTEP\n0\nITEM: NUMBER OF ATOMS\n";

    bigint total_np = 0;
    for (int isolid=0; isolid < domain->solids.size(); isolid++) total_np += domain->solids[isolid]->np;

    dumpstream << total_np << endl;
    dumpstream << "ITEM: BOX BOUNDS sm sm sm\n";
    dumpstream << domain->boxlo[0] << " " << domain->boxhi[0] << endl;
    dumpstream << domain->boxlo[1] << " " << domain->boxhi[1] << endl;
    dumpstream << domain->boxlo[2] << " " << domain->boxhi[2] << endl;
    dumpstream << "ITEM: ATOMS id type ";
    for (auto v: output_var) {
      dumpstream << v << " ";
    }
    dumpstream << endl;

    bigint ID = 0;
    for (int isolid=0; isolid < domain->solids.size(); isolid++) {
      Solid *s = domain->solids[isolid];
      for (bigint i=0; i<s->np;i++) {
	ID++;
	dumpstream << i << " ";
	dumpstream << isolid+1 << " ";
	for (auto v: output_var) {
	  if (v.compare("x")==0) dumpstream << s->x[i][0] << " ";
	  else if (v.compare("y")==0) dumpstream << s->x[i][1] << " ";
	  else if (v.compare("z")==0) dumpstream << s->x[i][2] << " ";
	  else if (v.compare("x0")==0) dumpstream << s->x0[i][0] << " ";
	  else if (v.compare("y0")==0) dumpstream << s->x0[i][1] << " ";
	  else if (v.compare("z0")==0) dumpstream << s->x0[i][2] << " ";
	  else if (v.compare("vx")==0) dumpstream << s->v[i][0] << " ";
	  else if (v.compare("vy")==0) dumpstream << s->v[i][1] << " ";
	  else if (v.compare("vz")==0) dumpstream << s->v[i][2] << " ";
	  else if (v.compare("s11")==0) dumpstream << s->sigma[i](0,0) << " ";
	  else if (v.compare("s22")==0) dumpstream << s->sigma[i](1,1) << " ";
	  else if (v.compare("s33")==0) dumpstream << s->sigma[i](2,2) << " ";
	  else if (v.compare("s12")==0) dumpstream << s->sigma[i](0,1) << " ";
	  else if (v.compare("s13")==0) dumpstream << s->sigma[i](0,2) << " ";
	  else if (v.compare("s23")==0) dumpstream << s->sigma[i](1,2) << " ";
	  else if (v.compare("seq")==0) dumpstream << sqrt(3. / 2.) * Deviator(s->sigma[i]).norm() << " ";
	  else if (v.compare("damage")==0) dumpstream << s->damage[i] << " ";
	  else if (v.compare("damage_init")==0) dumpstream << s->damage_init[i] << " ";
	  else if (v.compare("volume")==0) dumpstream << s->vol[i] << " ";
	  else if (v.compare("mass")==0) dumpstream << s->mass[i] << " ";
	  else if (v.compare("bx")==0) dumpstream << s->mb[i][0] << " ";
	  else if (v.compare("by")==0) dumpstream << s->mb[i][1] << " ";
	  else if (v.compare("bz")==0) dumpstream << s->mb[i][2] << " ";
	  else if (v.compare("ep")==0) dumpstream << s->eff_plastic_strain[i] << " ";
	  else if (v.compare("epdot")==0) dumpstream << s->eff_plastic_strain_rate[i] << " ";
	}
	dumpstream << endl;
      }
    }
    dumpstream.close();
  } else {
    cout << "Error: cannot write in file: " << fdump << endl;
    exit(1);
  }
}
