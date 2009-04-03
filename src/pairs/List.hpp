#ifndef _PAIRS_LIST_HPP
#define _PAIRS_LIST_HPP

#include "Set.hpp"
#include "Computer.hpp"
#include "particles/ParticleHandle.hpp"
#include "particles/Storage.hpp"
#include "bc/BC.hpp"

namespace espresso {
  namespace pairs {

    /** Class that applies a Computer to a list of pairs 
     */

    class List : public Set {
 
    private:

      particles::Storage& storage; 
      bc::BC& bc;
      particles::PropertyId coordinates;
      typedef std::pair<particles::ParticleId,
                        particles::ParticleId> Tuple;
      std::vector<Tuple> id_list;

    public:
      /** Destructor. */
      ~List();

      /** Constructor for this class 

	  \param bc are the boundary conditions that are needed for distance calculation.
	  \param storage specifies the particle storage to which particles belong
	  \param coordinates the identifier of the coordinates property to use

      */
      List (bc::BC& bc, 
	    particles::Storage& storage, 
	    particles::PropertyId coordinates);

      size_t size();

      /** Ask if a particle pair tuple (id1, id2) is in the pair list

	  \param id1 is the identificaiton of particle 1
	  \param id2 is the identificaiton of particle 2
	  \return true if tuple (id1, id2) is in the list
 
      */

      bool findPair(particles::ParticleId id1, particles::ParticleId id2);

      /** Adding a particle pair tuple (id1, id2) to the pair list

	  \param id1 is the identificaiton of particle 1
	  \param id2 is the identificaiton of particle 2
 
	  Note: a tuple (id1, id2) can be added several times.
      */

      void addPair(particles::ParticleId id1, particles::ParticleId id2);

      /** Deleting a particle pair tuple (id1, id2) from the pair list

	  \param id1 is the identificaiton of particle 1
	  \param id2 is the identificaiton of particle 2
 
	  Particle (id1, i2) must be in the pair list otherwise exception.
      */

      void deletePair(particles::ParticleId id1, particles::ParticleId id2);

      /** Getter routine for the boundary conditions. */

      bc::BC& getBC() const { return bc; }

      /** Getter routine for the ID of the coordinate */

      particles::PropertyId getCoordinateProperty() const {return coordinates; }

      /** This routine will apply a function operator to all pairs.

	  \param pairComputer is the object that provides the function to be applied to all pairs.

      */

      virtual void foreach(Computer& pairComputer);

      /** This routine will apply a function operator for read-only particles to all pairs.

	  \param pairComputer is the object that provides the read-only function to be applied to all pairs.

      */

      virtual void foreach(ConstComputer& pairComputer) const;

    };
  }
}

#endif
