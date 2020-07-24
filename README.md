# DREAM
This directory contains the Disruption Runaway Electron Avoidance Model (DREAM) code.

## Requirements
To compile DREAM, you need to have the following software installed:

- [CMake](https://cmake.org/) >= 3.12
- A C++17 compatible compiler (such as gcc >= 7.0)
- [GNU Scientific Library](https://www.gnu.org/software/gsl/) >= 2.0
- [HDF5](https://www.hdfgroup.org/)
- [PETSc](https://www.mcs.anl.gov/petsc)
- OpenMP
- MPI (for PETSc)
- Python 3 (required for generating ADAS data)

Additionally, to use the DREAM Python interface, you need the following
Python packages:

- h5py
- matplotlib
- numpy

### Notes on PETSc
While most of the software required by DREAM can be installed directly from
your Linux distribution's package repository, PETSc usually requires a manual
setup. To install PETSc, grab it's sources from the PETSc website or clone the
PETSc git repository:
```bash
$ git clone -b maint https://gitlab.com/petsc/petsc.git petsc
```
After this, compiling PETSc should be a matter of running the following
commands:
```bash
$ ./configure PETSC_ARCH=linux-c-opt
...
$ make PETSC_DIR=/path/to/petsc PETSC_ARCH=linux-c-opt all
...
```
Optionally, you can also run ``make check`` after ``make all``.

Once PETSc has been compiled with the above commands, you only need to make sure
that DREAM will be able to find your PETSc installation. The easiest way to
achieve this is to add the ``PETSC_DIR`` and ``PETSC_ARCH`` environment
variables used above to your ``~/.bashrc`` file (if you use bash; if you're
unsure, you probably do):
```bash
...
export PETSC_DIR="/path/to/petsc"
export PETSC_ARCH=linux-c-opt
```
Of course, the value for ``PETSC_DIR`` should be modified according to where
you installed PETSc. An alternative to modifying your ``~/.bashrc`` file is to
just give these variables directly to CMake every time you reconfigure DREAM
(which is usually not very often, unless you're a DREAM developer).

## Compilation
To compile DREAM, go to the root DREAM directory and run the following commands:

```bash
$ mkdir -p build
$ cd build
$ cmake ..
$ make -j NTHREADS
```
where ``NTHREADS`` is the number of CPU threads on your computer. If CMake can't
find PETSc, you can change the ``cmake`` command above to read
```bash
$ cmake .. -DPETSC_DIR=/path/to/petsc -DPETSC_ARCH=linux-c-opt
```
where ``/path/to/petsc`` is the path to the directory containing your PETSc
installation.

