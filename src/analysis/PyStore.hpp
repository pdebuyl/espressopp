/*
  Copyright (C) 2015
      Pierre de Buyl

  This file is part of ESPResSo++.

  ESPResSo++ is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ESPResSo++ is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// ESPP_CLASS
#ifndef _ANALYSIS_PYSTORE_HPP
#define _ANALYSIS_PYSTORE_HPP

#include "types.hpp"
#include "SystemAccess.hpp"
#include <Python.h>
#include <object.h>

namespace espressopp {
  namespace analysis {

    template <class T> void init_pb(Py_buffer *pb, int ndim, int *shape);
    void free_pb(Py_buffer *pb);

    /** Store particle data in Python buffers */
    class PyStore : public SystemAccess {
    public:
      PyStore(shared_ptr<System> system);
      ~PyStore();

      int get_NLocal() { return NLocal; }
      void set_store_position(bool _s) { store_position=_s; }
      bool get_store_position() { return store_position; }
      void set_store_id(bool _s) { store_id=_s; }
      bool get_store_id() { return store_id; }
      void set_store_species(bool _s) { store_species=_s; }
      bool get_store_species() { return store_species; }
      void set_store_state(bool _s) { store_state=_s; }
      bool get_store_state() { return store_state; }
      void clear_buffers();

      void update();
      PyObject* getPosition();
      PyObject* getId();
      PyObject* getSpecies();
      PyObject* getState();

      static void registerPython();
    private:
      bool store_position, store_velocity, store_mass, store_id, store_force, store_species, store_state;
      bool cleared;
      int NLocal;
      Py_buffer position;
      Py_buffer velocity;
      Py_buffer mass;
      Py_buffer id;
      Py_buffer force;
      Py_buffer species;
      Py_buffer state;
    };
  }
}

#endif
