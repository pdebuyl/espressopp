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

#include "types.hpp"
#include "SystemAccess.hpp"
#include "storage/Storage.hpp"
#include <Python.h>
#include <object.h>
#include "PyStore.hpp"
#include "iterator/CellListIterator.hpp"
#include <iostream>

const char *get_format(double x) { return "d"; }
const char *get_format(float x) { return "f"; }
const char *get_format(int x) { return "i"; }

namespace espressopp {
  namespace analysis {

    /** Initialize a Py_buffer object
     */
    template <class T> void init_pb(Py_buffer *pb, int n, int d)
    {
      T dum=0;
      // Setting basic variable of the Py_buffer
      pb->suboffsets = NULL;
      pb->internal = NULL;
      pb->obj = NULL;
      pb->readonly = 1;
      pb->ndim = 2;
      // The format is computed as a function of the template type T
      pb->format = (char*)malloc(2*sizeof(const char));
      strcpy(pb->format, get_format(dum));
      // Allocation and setting of the shape, stride and len
      pb->shape = (Py_ssize_t *)malloc(pb->ndim*sizeof(Py_ssize_t));
      pb->strides = NULL;
      pb->shape[0] = n;
      pb->shape[1] = d;
      pb->itemsize = sizeof(T);
      pb->len = pb->itemsize;
      for (int i=0; i<pb->ndim; i++) {
	pb->len *= pb->shape[i];
      }
      pb->buf = malloc(pb->len);
    }

    void free_pb(Py_buffer *pb) {
      free(pb->format);
      free(pb->shape);
      free(pb->buf);
    }

    PyStore::PyStore(shared_ptr<System> system) : SystemAccess(system) {
      store_position = store_id = true;
      store_velocity = store_mass = store_force = store_species = false;
      cleared = true;
    }

    PyStore::~PyStore() {
      if (!cleared) clear_buffers();
    }

    void PyStore::clear_buffers() {
      if (!cleared) {
	if (store_position) free_pb(&position);
	if (store_id) free_pb(&id);
      }
    }

    void PyStore::update() {
      System& system = getSystemRef();

      int NLocal = system.storage->getNRealParticles();

      if (store_position) init_pb<real>(&position, NLocal, 3);
      if (store_id) init_pb<real>(&position, NLocal, 3);

      CellList realCells = system.storage->getRealCells();

      int i=0;
      real *tmpBuf = (real *) position.buf;
      for(iterator::CellListIterator cit(realCells); !cit.isDone(); ++cit) {
	Real3D &tmpPos = cit->position();
	tmpBuf[3*i] = tmpPos[0];
	tmpBuf[3*i+1] = tmpPos[1];
	tmpBuf[3*i+2] = tmpPos[2];
	i++;
      }
    }

    PyObject* PyStore::getPosition() {
      return PyMemoryView_FromBuffer(&position);
    }

    void PyStore::registerPython() {
      using namespace espressopp::python;

      class_<PyStore>
	("analysis_PyStore", init< shared_ptr< System > >())
	.def("update", &PyStore::update)
	.def("clear_buffers", &PyStore::clear_buffers)
	.def("getPosition", &PyStore::getPosition)
        .add_property("store_position", &PyStore::get_store_position, &PyStore::set_store_position)
        .add_property("store_id", &PyStore::get_store_id, &PyStore::set_store_id)

	;
    }

  }
}
