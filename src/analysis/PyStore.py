
from espressopp.esutil import cxxinit
from espressopp import pmi

from _espressopp import analysis_PyStore

class PyStoreLocal(analysis_PyStore):
    def __init__(self, system):
        cxxinit(self, analysis_PyStore, system)
    def update(self):
        self.cxxclass.update(self)
    def getPosition(self):
        return self.cxxclass.getPosition(self)

if pmi.isController:
    class PyStore(object):
        __metaclass__ = pmi.Proxy
        pmiproxydefs = dict(
            cls =  'espressopp.analysis.PyStoreLocal',
            pmicall = ['update', 'getPosition'],
        )
