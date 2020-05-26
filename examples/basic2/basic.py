#!/usr/bin/env python3
#
# A very basic DREAM Python example. This script generates a basic
# DREAM input file which can be passed to 'dreami'.
#
# Run as
#
#   $ ./basic.py
#
# ###################################################################

import numpy as np
import sys

sys.path.append('../../py/')

from DREAM.DREAMSettings import DREAMSettings
import DREAM.Settings.Equations.IonSpecies as Ions
import DREAM.Settings.Solver as Solver
import DREAM.Settings.MomentumGrid as Collisions


ds = DREAMSettings()

times  = [0]
radius = [0, 1]

# Set E_field
efield = 0*np.ones((len(times), len(radius)))
ds.eqsys.E_field.setPrescribedData(efield=efield, times=times, radius=radius)

# Set n_cold (prescribed; it is automatically calculated self-consistently otherwise)
#density = 1e20 * np.ones((len(times), len(radius)))
#ds.eqsys.n_cold.setPrescribedData(density=density, times=times, radius=radius)

# Set temperature
temperature = 1e4 * np.ones((len(times), len(radius)))
ds.eqsys.T_cold.setPrescribedData(temperature=temperature, times=times, radius=radius)

# Set ions
ds.eqsys.n_i.addIon(name='D', Z=1, iontype=Ions.IONS_PRESCRIBED_FULLY_IONIZED, n=1e20)
ds.eqsys.n_i.addIon(name='Ar', Z=18, iontype=Ions.IONS_PRESCRIBED_NEUTRAL, n=1e20)

# Hot-tail grid settings
pmax = 1.0
ds.hottailgrid.setNxi(10)
ds.hottailgrid.setNp(200)
ds.hottailgrid.setPmax(pmax)

#ds.hottailgrid.collfreq_mode = Collisions.COLLFREQ_MODE_SUPERTHERMAL
ds.hottailgrid.collfreq_mode = Collisions.COLLFREQ_MODE_FULL

# Set initial Maxwellian @ T = 1 keV, n = 5e19, uniform in radius
ds.eqsys.f_hot.setInitialProfiles(rn0=0, n0=5e19, rT0=0, T0=1000)

# Disable runaway grid
ds.runawaygrid.setEnabled(False)

# Set up radial grid
ds.radialgrid.setB0(5)
ds.radialgrid.setMinorRadius(0.22)
ds.radialgrid.setNr(10)

# Use the linear solver
ds.solver.setType(Solver.TYPE_LINEAR_IMPLICIT)

# Set time stepper
#ds.timestep.setTmax(1e-2)
#ds.timestep.setNt(100)
ds.timestep.setTmax(1e-7)
ds.timestep.setNt(10)

# Save settings to HDF5 file
ds.save('dream_settings.h5')

