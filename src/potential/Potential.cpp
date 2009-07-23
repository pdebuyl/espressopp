#include "python.hpp"

#include "Potential.hpp"

using namespace espresso;
using namespace espresso::potential;

pairs::Computer::SelfPtr 
Potential::createForceComputer(Property< Real3D >::SelfPtr _forceProperty) {
  return createForceComputer(_forceProperty, _forceProperty);
}

pairs::Computer::SelfPtr 
Potential::createEnergyComputer(Property< real >::SelfPtr _energyProperty) {
  return createEnergyComputer(_energyProperty, _energyProperty);
}

//////////////////////////////////////////////////
// REGISTRATION WITH PYTHON
//////////////////////////////////////////////////
class PythonPotential 
  : public Potential, 
    public espresso::python::wrapper<Potential> {
public:
  virtual real getCutoffSqr() const {
    return get_override("getCutoffSqr")();
  }

  virtual real computeEnergy(const Real3D dist) const {
    return get_override("computeEnergy")(dist);
  }

  virtual Real3D computeForce(const Real3D dist) const {
    return get_override("computeForce")(dist);
  }
};

void
Potential::registerPython() {
  using namespace espresso::python;

  pairs::Computer::SelfPtr (Potential::*createForceComputer1)
    (Property< Real3D >::SelfPtr, Property< Real3D >::SelfPtr) 
     = &Potential::createForceComputer;
  pairs::Computer::SelfPtr (Potential::*createForceComputer2)
    (Property< Real3D >::SelfPtr) 
     = &Potential::createForceComputer;

  pairs::Computer::SelfPtr (Potential::*createEnergyComputer1)
    (Property< real >::SelfPtr, Property< real >::SelfPtr) 
     = &Potential::createEnergyComputer;
  pairs::Computer::SelfPtr (Potential::*createEnergyComputer2)
    (Property< real >::SelfPtr) 
     = &Potential::createEnergyComputer;

  class_< PythonPotential, boost::noncopyable >
    ("potential_PythonPotential", no_init)
    .def("getCutoffSqr", pure_virtual(&Potential::getCutoffSqr))
    .def("computeEnergy", pure_virtual(&Potential::computeEnergy))
    .def("computeForce", pure_virtual(&Potential::computeForce))
    .def("createForceComputer", pure_virtual(createForceComputer1))
    .def("createForceComputer", createForceComputer2)
    .def("createEnergyComputer", pure_virtual(createEnergyComputer1))
    .def("createEnergyComputer", createEnergyComputer2)
    ;
}
