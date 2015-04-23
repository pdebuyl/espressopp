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
    template <class T> void init_pb(Py_buffer *pb, int ndim, int *shape)
    {
      T dum=0;
      // Setting basic variable of the Py_buffer
      pb->suboffsets = NULL;
      pb->internal = NULL;
      pb->obj = NULL;
      pb->readonly = 1;
      pb->ndim = ndim;
      // The format is computed as a function of the template type T
      pb->format = (char*)malloc(2*sizeof(const char));
      strcpy(pb->format, get_format(dum));
      // Allocation and setting of the shape, stride and len
      pb->shape = (Py_ssize_t *)malloc(pb->ndim*sizeof(Py_ssize_t));
      pb->strides = NULL;
      pb->itemsize = sizeof(T);
      pb->len = pb->itemsize;
      for (int i=0; i<pb->ndim; i++) {
	pb->shape[i] = shape[i];
	pb->len *= pb->shape[i];
      }
      pb->buf = malloc(pb->len);
    }

    void free_pb(Py_buffer *pb) {
      free(pb->format);
      pb->format = NULL;
      free(pb->shape);
      pb->shape = NULL;
      free(pb->buf);
      pb->buf = NULL;
      pb->len = 0;
    }

    PyStore::PyStore(shared_ptr<System> system) : SystemAccess(system) {
      store_position = store_id = true;
      store_velocity = store_mass = store_force = store_species = false;
      cleared = true;
      NLocal = -1;
      position.len = 0;
      velocity.len = 0;
      mass.len = 0;
      id.len = 0;
      force.len = 0;
      species.len = 0;
    }

    PyStore::~PyStore() {
      clear_buffers();
    }

    void PyStore::clear_buffers() {
      if (!cleared) {
	if (store_position) free_pb(&position);
	if (store_id) free_pb(&id);
	cleared = true;
      }
    }

    void PyStore::update() {
      System& system = getSystemRef();
      int shape[2];

      clear_buffers();

      NLocal = system.storage->getNRealParticles();

      shape[0] = NLocal;
      shape[1] = 3;

      if (store_position) init_pb<real>(&position, 2, shape);
      if (store_id) init_pb<longint>(&id, 1, shape);
      cleared = false;

      CellList realCells = system.storage->getRealCells();

      int i=0;
      for(iterator::CellListIterator cit(realCells); !cit.isDone(); ++cit) {
	if (store_position) {
	  Real3D &tmpPos = cit->position();
	  ((real *) position.buf)[3*i] = tmpPos[0];
	  ((real *) position.buf)[3*i+1] = tmpPos[1];
	  ((real *) position.buf)[3*i+2] = tmpPos[2];
	}
	if (store_id) ((longint *) id.buf)[i] = cit->getId();
	i++;
      }
    }

    PyObject* PyStore::getPosition() {
      if (store_position && position.len) return PyMemoryView_FromBuffer(&position);
      Py_INCREF(Py_None);
      return Py_None;
    }

    PyObject* PyStore::getId() {
      if (store_id && id.len) return PyMemoryView_FromBuffer(&id);
      Py_INCREF(Py_None);
      return Py_None;
    }

    void PyStore::registerPython() {
      using namespace espressopp::python;

      class_<PyStore>
	("analysis_PyStore", init< shared_ptr< System > >())
	.def("update", &PyStore::update)
	.def("clear_buffers", &PyStore::clear_buffers)
	.def("getPosition", &PyStore::getPosition)
	.def("getId", &PyStore::getId)
        .add_property("NLocal", &PyStore::get_NLocal)
        .add_property("store_position", &PyStore::get_store_position, &PyStore::set_store_position)
        .add_property("store_id", &PyStore::get_store_id, &PyStore::set_store_id)

	;
    }

  }
}
