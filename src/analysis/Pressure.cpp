#include <cmath>
#include <python.hpp>
#include "Pressure.hpp"
#include "storage/DomainDecomposition.hpp"
#include "iterator/CellListIterator.hpp"
#include "interaction/Interaction.hpp"
#include "interaction/Potential.hpp"
#include "bc/BC.hpp"
#include "VerletList.hpp"
#include "mpi.hpp"

using namespace espresso;
using namespace iterator;
using namespace interaction;

namespace espresso {
  namespace analysis {
    real Pressure::compute() const {

      System& system = getSystemRef();
  
      // determine number of local particles and total particles
      int N;
      int Nsum;
      N = system.storage->getNRealParticles();
      boost::mpi::reduce(*mpiWorld, N, Nsum, std::plus<int>(), 0);

      // compute the kinetic contriubtion (2/3 \sum 1/2mv^2)
      real e_kinetic;
      real p_kinetic;
      real v2sum;
      real v2 = 0.0;

      CellList realCells = system.storage->getRealCells();
      for (CellListIterator cit(realCells); !cit.isDone(); ++cit) {
        v2 = v2 + pow(cit->m.v[0], 2) + pow(cit->m.v[1], 2) + pow(cit->m.v[2], 2);
      }
      boost::mpi::reduce(*mpiWorld, v2, v2sum, std::plus<real>(), 0);
      e_kinetic = 0.5 * v2sum;
      p_kinetic = (2.0 / 3.0) * e_kinetic;

      // compute the short-range nonbonded contribution
      real rij_dot_Fij = 0.0;
      real p_nonbonded;

      // loop over interaction types
      const InteractionList& srIL = system.shortRangeInteractions;
      for (size_t j = 0; j < srIL.size(); j++) {
        real cut = srIL[j]->getMaxCutoff();

        // next line is a failed attempt to get the Verlet list
        // this fails because srIL stores Interactions and not
        // VerletListInteractions.
	// const PairList pairList = srIL[j]->getVerletList()->getPairs();
        /*
	int npairs = pairList.size();
	for (int i = 0; i < npairs; i++) {
	  Particle &p1 = *pairList[i].first;
	  Particle &p2 = *pairList[i].second;
	  int type1 = p1.p.type;
	  int type2 = p2.p.type;
	  const Potential &potential = getPotential(type1, type2);

	  Real3D force;
	  if (potential._computeForce(force, p1, p2))
	    for(int k = 0; k < 3; k++) {
	      p1.f.f[k] += force[k];
	      p2.f.f[k] -= force[k];
	    }
	  }
        */


      }
      Real3D Li = system.bc->getBoxL();
      real V = Li[0] * Li[1] * Li[2];
      p_nonbonded = -rij_dot_Fij / (3.0 * V);

      return (p_kinetic + p_nonbonded);
    }

    void Pressure::registerPython() {
      using namespace espresso::python;
      class_<Pressure, bases< Observable > >
        ("analysis_Pressure", init< shared_ptr< System > >())
      ;
    }
  }
}