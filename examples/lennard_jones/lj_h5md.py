
# This file is based on examples/lennard_jones/lennard_jones.py from the
# espressopp distribution

import espressopp

import sys
import time

########################################################################
# 1. specification of the main simulation parameters                   #
########################################################################

# number of particles
Npart              = 2048*1
rho                = 0.8442
L                  = pow(Npart/rho, 1.0/3.0)
box                = (L, L, L)
r_cutoff           = 2.5
skin               = 0.4
temperature        = 1.0
dt                 = 0.004
epsilon            = 1.0
sigma              = 1.0

# interaction cut-off used during the warm-up phase
warmup_cutoff      = pow(2.0, 1.0/6.0)
# number of warm-up loops
warmup_nloops      = 100
# number of integration steps performed in each warm-up loop
warmup_isteps      = 100
# total number of integration steps of the warm-up phase
total_warmup_steps = warmup_nloops * warmup_isteps
# initial value for LJ epsilon at beginning of warmup
epsilon_start      = 0.1
# final value for LJ epsilon at end of warmup
epsilon_end        = 1.0
# increment epsilon by epsilon delta after each warmup_loop
epsilon_delta      = (epsilon_end - epsilon_start) / warmup_nloops
# force capping radius
capradius          = 0.8
# number of equilibration loops
equil_nloops       = 100
# number of integration steps performed in each equilibration loop
equil_isteps       = 50

# print ESPResSo++ version and compile info
print espressopp.Version().info()

########################################################################
# 2. setup of the system, random number geneartor and parallelisation  #
########################################################################

# create the basic system
system             = espressopp.System()
# use the random number generator that is included within the ESPResSo++ package
system.rng         = espressopp.esutil.RNG()
# use orthorhombic periodic boundary conditions 
system.bc          = espressopp.bc.OrthorhombicBC(system.rng, box)
# set the skin size used for verlet lists and cell sizes
system.skin        = skin
# get the number of CPUs to use
NCPUs              = espressopp.MPI.COMM_WORLD.size
# calculate a regular 3D grid according to the number of CPUs available
nodeGrid           = espressopp.tools.decomp.nodeGrid(NCPUs)
# calculate a 3D subgrid to speed up verlet list builds and communication
cellGrid           = espressopp.tools.decomp.cellGrid(box, nodeGrid, warmup_cutoff, skin)
# create a domain decomposition particle storage with the calculated nodeGrid and cellGrid
system.storage     = espressopp.storage.DomainDecomposition(system, nodeGrid, cellGrid)

print "NCPUs              = ", NCPUs
print "nodeGrid           = ", nodeGrid
print "cellGrid           = ", cellGrid

########################################################################
# 3. setup of the integrator and simulation ensemble                   #
########################################################################

# use a velocity Verlet integration scheme
integrator     = espressopp.integrator.VelocityVerlet(system)
# set the integration step  
integrator.dt  = dt
# use a thermostat if the temperature is set
if (temperature != None):
  # create e Langevin thermostat
  thermostat             = espressopp.integrator.LangevinThermostat(system)
  # set Langevin friction constant
  thermostat.gamma       = 1.0
  # set temperature
  thermostat.temperature = temperature
  # tell the integrator to use this thermostat
  integrator.addExtension(thermostat)

## steps 2. and 3. could be short-cut by the following expression:
## system, integrator = espressopp.standard_system.Default(box, warmup_cutoff, skin, dt, temperature)

########################################################################
# 4. adding the particles                                              #
########################################################################

print "adding ", Npart, " particles to the system ..." 
for pid in range(Npart):
  # get a 3D random coordinate within the box
  pos = system.bc.getRandomPos()
  # add a particle with particle id pid and coordinate pos to the system
  # coordinates are automatically folded according to periodic boundary conditions
  # the following default values are set for each particle:
  # (type=0, mass=1.0, velocity=(0,0,0), charge=0.0)
  system.storage.addParticle(pid, pos)
# distribute the particles to parallel CPUs 
system.storage.decompose()

########################################################################
# 5. setting up interaction potential for the warmup                   #
########################################################################

# create a verlet list that uses a cutoff radius = warmup_cutoff
# the verlet radius is automatically increased by system.skin (see system setup)
verletlist  = espressopp.VerletList(system, warmup_cutoff)
# create a force capped Lennard-Jones potential
# the potential is automatically shifted so that U(r=cutoff) = 0.0
LJpot       = espressopp.interaction.LennardJonesCapped(epsilon=epsilon_start, sigma=sigma, cutoff=warmup_cutoff, caprad=capradius, shift='auto')
# create a force capped Lennard-Jones interaction that uses a verlet list 
interaction = espressopp.interaction.VerletListLennardJonesCapped(verletlist)
# tell the interaction to use the above defined force capped Lennard-Jones potential
# between 2 particles of type 0 
interaction.setPotential(type1=0, type2=0, potential=LJpot)



########################################################################
# 6. running the warmup loop
########################################################################

# make the force capping interaction known to the system
system.addInteraction(interaction)
print "starting warm-up ..."
# print some status information (time, measured temperature, pressure,
# pressure tensor (xy only), kinetic energy, potential energy, total energy, boxsize)
espressopp.tools.info(system, integrator)

for step in range(warmup_nloops):
  # perform warmup_isteps integraton steps
  integrator.run(warmup_isteps)
  # decrease force capping radius in the potential
  LJpot.epsilon += epsilon_delta
  # update the type0-type0 interaction to use the new values of LJpot
  interaction.setPotential(type1=0, type2=0, potential=LJpot)
  # print status info
  espressopp.tools.info(system, integrator)  

print "warmup finished"
# remove the force capping interaction from the system
system.removeInteraction(0) 
# the equilibration uses a different interaction cutoff therefore the current
# verlet list is not needed any more and would waste only CPU time
verletlist.disconnect()

########################################################################
# 7. setting up interaction potential for the equilibration            #
########################################################################

# create a new verlet list that uses a cutoff radius = r_cutoff
# the verlet radius is automatically increased by system.skin (see system setup)
verletlist  = espressopp.VerletList(system, r_cutoff)
# define a Lennard-Jones interaction that uses a verlet list 
interaction = espressopp.interaction.VerletListLennardJones(verletlist)
# use a Lennard-Jones potential between 2 particles of type 0 
# the potential is automatically shifted so that U(r=cutoff) = 0.0
# if the potential should not be shifted set shift=0.0
potential   = interaction.setPotential(type1=0, type2=0,
                                       potential=espressopp.interaction.LennardJones(
                                       epsilon=epsilon, sigma=sigma, cutoff=r_cutoff, shift=0.0))

########################################################################
# 8. running the equilibration loop                                    #
########################################################################

# add the new interaction to the system
system.addInteraction(interaction)
# since the interaction cut-off changed the size of the cells that are used
# to speed up verlet list builds should be adjusted accordingly 
system.storage.cellAdjust()

# set all integrator timers to zero again (they were increased during warmup)
integrator.resetTimers()
# set integrator time step to zero again
integrator.step = 0

print "starting equilibration ..."
# print inital status information
espressopp.tools.info(system, integrator)
for step in range(equil_nloops):
  # perform equilibration_isteps integration steps
  integrator.run(equil_isteps)
  # print status information
  espressopp.tools.info(system, integrator)

print "equilibration finished"

ps = espressopp.analysis.PyStore(system, 'mypsdump.h5', store_species=True, store_state=True)

for step in range(equil_nloops):
  # perform equilibration_isteps integration steps
  integrator.run(equil_isteps)
  # print status information
  espressopp.tools.info(system, integrator)
  ps.dump(integrator.step, integrator.step*integrator.dt)

ps.close_file()