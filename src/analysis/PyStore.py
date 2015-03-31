
from espressopp.esutil import cxxinit
from espressopp import pmi
from _espressopp import analysis_PyStore

from mpi4py import MPI
import h5py
import numpy as np

class PyStoreLocal(analysis_PyStore):
    def __init__(self, system):
        cxxinit(self, analysis_PyStore, system)
    def update(self):
        self.cxxclass.update(self)
    def getPosition(self):
        return self.cxxclass.getPosition(self)
    def open_file(self, filename):
        self.file = h5py.File(filename, 'w', driver='mpio', comm=MPI.COMM_WORLD)
    def dump(self):
        self.update()
        pos_data = self.getPosition()
        pos = np.asarray(pos_data)
        NLocal = np.array(pos.shape[0], 'i')
        NMaxLocal = np.array(0, 'i')
        MPI.COMM_WORLD.Allreduce(NLocal, NMaxLocal, op=MPI.MAX)
        cpu_size = ((NMaxLocal//256)+1)*256
        print NLocal, NMaxLocal
        total_size = MPI.COMM_WORLD.size*cpu_size
        self.file.create_dataset('position', (total_size,3), dtype=np.float64, chunks=(256,3))
        idx_0 = MPI.COMM_WORLD.rank*cpu_size
        idx_1 = idx_0+NLocal
        self.file['position'][idx_0:idx_1] = pos
        self.file.create_dataset('species', (total_size,), dtype=int, chunks=(256,))
        self.file['species'][idx_0:idx_1] = 1

    def close_file(self):
        self.file.close()

if pmi.isController:
    class PyStore(object):
        __metaclass__ = pmi.Proxy
        pmiproxydefs = dict(
            cls =  'espressopp.analysis.PyStoreLocal',
            pmicall = ['update', 'getPosition', 'open_file', 'close_file', 'dump'],
        )
