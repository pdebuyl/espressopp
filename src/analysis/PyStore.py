
import espressopp
from espressopp.esutil import cxxinit
from espressopp import pmi
from _espressopp import analysis_PyStore

from mpi4py import MPI
import h5py
import numpy as np
import pyh5md

class PyStoreLocal(analysis_PyStore):
    def __init__(self, system, filename, **kwargs):
        cxxinit(self, analysis_PyStore, system)
        self.system = system
        self.file = pyh5md.H5MD_File(filename, 'w', driver='mpio', comm=MPI.COMM_WORLD,
                                     creator='espressopp',
                                     creator_version=espressopp.VersionLocal().info(),
                                     author='Pierre de Buyl',
                                     )
        for k, v in kwargs.iteritems():
            if k in ['store_position', 'store_id', 'store_species', 'store_state']:
                self.__setattr__(k, v)
        part = self.file.particles_group('all')
        self.box = part.box(dimension=3, boundary=['periodic', 'periodic', 'periodic'], time=True, edges=np.zeros(3, dtype=np.float64))
        if self.store_id:
            self.id_e = part.trajectory('id', (256,), np.int, chunks=(1,256), fillvalue=-1)
        if self.store_position:
            self.position = part.trajectory('position', (256,3), np.float64, chunks=(1,256, 3))
        if self.store_species:
            self.species = part.trajectory('species', (256,), np.int, chunks=(1,256), fillvalue=-1)
        if self.store_state:
            self.state = part.trajectory('state', (256,), np.int, chunks=(1,256), fillvalue=-1)

    def update(self):
        self.cxxclass.update(self)
    def clear_buffers(self):
        self.cxxclass.clear_buffers(self)
    def getPosition(self):
        return self.cxxclass.getPosition(self)
    def getId(self):
        return self.cxxclass.getId(self)
    def getSpecies(self):
        return self.cxxclass.getSpecies(self)
    def getState(self):
        return self.cxxclass.getState(self)
    def dump(self, step, time):
        self.update()
        NLocal = np.array(self.NLocal, 'i')
        NMaxLocal = np.array(0, 'i')
        MPI.COMM_WORLD.Allreduce(NLocal, NMaxLocal, op=MPI.MAX)
        cpu_size = ((NMaxLocal//256)+1)*256
        print NLocal, NMaxLocal
        total_size = MPI.COMM_WORLD.size*cpu_size
        idx_0 = MPI.COMM_WORLD.rank*cpu_size
        idx_1 = idx_0+NLocal
        if self.store_position:
            pos = np.asarray(self.getPosition())
            if total_size>self.position.value.shape[1]:
                self.position.value.resize(total_size, axis=1)
            self.position.append(pos, step, time, region=(idx_0, idx_1))
            self.box.edges.append(np.array([edge_i for edge_i in self.system.bc.boxL], dtype=np.float64), step, time)
        if (self.store_id):
            id_ar = np.asarray(self.getId())
            if total_size>self.id_e.value.shape[1]:
                self.id_e.value.resize(total_size, axis=1)
            self.id_e.append(id_ar, step, time, region=(idx_0, idx_1))
        if (self.store_species):
            species = np.asarray(self.getSpecies())
            if total_size>self.species.value.shape[1]:
                self.species.value.resize(total_size, axis=1)
            self.species.append(id_ar, step, time, region=(idx_0, idx_1))
        if (self.store_state):
            state = np.asarray(self.getState())
            if total_size>self.state.value.shape[1]:
                self.state.value.resize(total_size, axis=1)
            self.state.append(id_ar, step, time, region=(idx_0, idx_1))

    def close_file(self):
        self.file.close()

if pmi.isController:
    class PyStore(object):
        __metaclass__ = pmi.Proxy
        pmiproxydefs = dict(
            cls =  'espressopp.analysis.PyStoreLocal',
            pmicall = ['update', 'getPosition', 'getId', 'getSpecies', 'getState', 'close_file', 'dump', 'clear_buffers'],
            pmiproperty = ['store_position', 'store_id', 'store_species', 'store_state'],
        )
