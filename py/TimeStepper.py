#
# TimeStepper settings object
# #################################

import numpy as np


class TimeStepper:
    
    TYPE_CONSTANT = 1

    def __init__(self, ttype=1, tmax=None, dt=None, nt=None):
        """
        Constructor.
        """
        self.set(ttype=ttype, tmax=tmax, dt=dt, nt=nt)
        

    def checkConsistency(self):
        """
        Verify that the TimeStepper settings are consistent.
        """
        if self.type == self.TYPE_CONSTANT:
            if self.tmax is None or self.tmax <= 0:
                raise DREAMException("TimeStepper constant: 'tmax' must be set to a value > 0.")
            
            # Verify that _exactly_ one of 'dt' and 'nt' is
            # set to a valid value
            dtSet = (self.dt is not None and self.dt > 0)
            ntSet = (self.nt is not None and self.nt > 0)

            if dtSet and ntSet:
                raise DREAMException("TimeStepper constant: Exactly one of 'dt' and 'nt' must be > 0.")
        else:
            raise DREAMException("Unrecognized time stepper type selected: {}.".format(self.type))


    def set(self, ttype=1, tmax=None, dt=None, nt=None):
        """
        Set properties of the time stepper.
        """
        self.type = int(ttype)
        self.tmax = None if tmax is None else self.setTmax(tmax)
        self.dt   = None if dt is None else self.setDt(dt)
        self.nt   = None if nt is None else self.setNt(nt)


    ######################
    # SETTERS
    ######################
    def setDt(self, dt):
        if dt <= 0:
            raise DREAMException("TimeStepper: Invalid value assigned to 'dt': {}".format(tmax))
        if self.nt is not None and self.dt > 0:
            raise DREAMException("TimeStepper: 'dt' may not be set alongside 'nt'.")
            
        self.dt = float(dt)


    def setNt(self, nt):
        if nt <= 0:
            raise DREAMException("TimeStepper: Invalid value assigned to 'dt': {}".format(tmax))
        if self.dt is not None and self.dt > 0:
            raise DREAMException("TimeStepper: 'nt' may not be set alongside 'dt'.")
            
        self.nt = int(nt)


    def setTmax(self, tmax):
        if tmax <= 0:
            raise DREAMException("TimeStepper: Invalid value assigned to 'tmax': {}".format(tmax))

        self.tmax = float(tmax)


    def todict(self):
        """
        Returns a Python dictionary containing all settings of
        this TimeStepper object.
        """
        self.checkConsistency()

        data = {
            'type': self.type,
            'tmax': self.tmax
        }

        if self.dt is not None: data['dt'] = self.dt
        if self.nt is not None: data['nt'] = self.nt

        return data


