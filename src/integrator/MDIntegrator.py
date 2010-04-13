from espresso import pmi
from _espresso import integrator_MDIntegrator

class MDIntegratorLocal(object):
    """Abstract local base class for molecular dynamics integrator."""
    def run(self, niter):
        return self.cxxclass.run(self, niter)

if pmi.isController :
    class MDIntegrator(object):
        """Abstract base class for molecular dynamics integrator."""
        __metaclass__ = pmi.Proxy
        pmiproxydefs = dict(
            pmiproperty = [ "dt" ],
            pmicall = [ "run" ]
            )