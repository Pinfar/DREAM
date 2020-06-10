
import numpy as np
from . EquationException import EquationException
from . PrescribedParameter import PrescribedParameter


TYPE_PRESCRIBED = 1
TYPE_SELFCONSISTENT = 2


class ElectricField(PrescribedParameter):
    
    def __init__(self, ttype=1, efield=None, radius=0, times=0):
        """
        Constructor.
        """
        self.setType(ttype=ttype)

        # Prescribed electric field evolution
        self.efield = None
        self.radius = None
        self.times  = None

        if (ttype == TYPE_PRESCRIBED) and (efield is not None):
            self.setPrescribedData(efield=efield, radius=radius, times=times)
        elif ttype == TYPE_SELFCONSISTENT:
            self.setType(ttype)

    def __getitem__(self, index):
        """
        Returns the value of the prescribed electric field at
        the given indices.
        """
        return self.efield[index]


    ####################
    # SETTERS
    ####################
    def setPrescribedData(self, efield, radius=0, times=0):
        _t, _rad, _tim = self._setPrescribedData(efield, radius, times)
        self.efield = _t
        self.radius = _rad
        self.times  = _tim

        self.verifySettingsPrescribedData()


    def setType(self, ttype):
        if ttype == TYPE_PRESCRIBED:
            self.type = ttype
        elif ttype == TYPE_SELFCONSISTENT:
            self.type = ttype
        else:
            raise EquationException("E_field: Unrecognized electric field type: {}".format(self.type))


    def fromdict(self, data):
        """
        Sets this paramater from settings provided in a dictionary.
        """
        self.type = data['type']

        if self.type == TYPE_PRESCRIBED:
            self.efield = data['data']['x']
            self.radius = data['data']['r']
            self.times  = data['data']['t']
        elif self.type == TYPE_SELFCONSISTENT:
            pass
        else:
            raise EquationException("E_field: Unrecognized electric field type: {}".format(self.type))

        self.verifySettings()


    def todict(self):
        """
        Returns a Python dictionary containing all settings of
        this ColdElectrons object.
        """
        data = { 'type': self.type }

        if self.type == TYPE_PRESCRIBED:
            data['data'] = {
                'x': self.efield,
                'r': self.radius,
                't': self.times
            }
        elif self.type == TYPE_SELFCONSISTENT:
            pass
        else:
            raise EquationException("E_field: Unrecognized electric field type: {}".format(self.type))

        return data


    def verifySettings(self):
        """
        Verify that the settings of this unknown are correctly set.
        """
        if self.type == TYPE_PRESCRIBED:
            if type(self.efield) != np.ndarray:
                raise EquationException("E_field: Electric field prescribed, but no electric field data provided.")
            elif type(self.times) != np.ndarray:
                raise EquationException("E_field: Electric field prescribed, but no time data provided, or provided in an invalid format.")
            elif type(self.radius) != np.ndarray:
                raise EquationException("E_field: Electric field prescribed, but no radial data provided, or provided in an invalid format.")

            self.verifySettingsPrescribedData()
        elif self.type == TYPE_SELFCONSISTENT:
            # Nothing todo
            pass
        else:
            raise EquationException("E_field: Unrecognized equation type specified: {}.".format(self.type))


    def verifySettingsPrescribedData(self):
        self._verifySettingsPrescribedData('E_field', self.efield, self.radius, self.times)
        """
        if len(self.efield.shape) != 2:
            raise EquationException("E_field: Invalid number of dimensions in prescribed data. Expected 2 dimensions (time x radius).")
        elif len(self.times.shape) != 1:
            raise EquationException("E_field: Invalid number of dimensions in time grid of prescribed data. Expected one dimension.")
        elif len(self.radius.shape) != 1:
            raise EquationException("E_field: Invalid number of dimensions in radial grid of prescribed data. Expected one dimension.")
        elif self.efield.shape[0] != self.times.size or self.efield.shape[1] != self.radius.size:
            raise EquationException("E_field: Invalid dimensions of prescribed data: {}x{}. Expected {}x{} (time x radius)."
                .format(self.efield.shape[0], self.efield.shape[1], self.times.size, self.radius.size))
        """


