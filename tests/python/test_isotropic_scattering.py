#! python3

from numpy import array
from pydisort import disort, get_legendre_coefficients
import os, unittest

# cdisort test01
class PyDisortTests(unittest.TestCase):
    def setUp(self):
        self.toml_path = "isotropic_scattering.toml"
        assert os.path.exists(self.toml_path), f"{self.toml_path} does not exist."
    
    ### cdisort test01
    def test_isotropic_scattering(self):
        ds = disort.from_file(self.toml_path)

        # set dimension
        ds.set_atmosphere_dimension(
            nlyr = 1, nstr = 16, nnmom = 16, nphase = 16)
        ds.set_itensity_dimension(
            nphi = 1, numu = 6, ntau = 2)
        ds.finalize()

        # get scattering moments
        pmom = get_legendre_coefficients(0., ds.nmom, model = "isotropic")

        # set boundary conditions
        ds.umu0 = 0.1
        ds.phi0 = 0.
        ds.albedo = 0.
        ds.fluor = 0.

        # set output optical depth and polar angles
        umu = array([-1., -0.5, -0.1, 0.1, 0.5, 1.0])
        uphi = array([0.])

        # run all cases
        for case in range(ncase):
            if case == 1:
                ds.fbeam = pi/ds.umu0
                ds.fisot = 0.
                tau = array([0., 0.03125])
                ssa = array([0.2])
            else:
                pass

        ds.run_rt_intensity(ssa = ssa, tau = tau, pmom = pmom,
                            umu = umu, uphi = uphi, utau = utau)
        assert ds.rfldir[0] == 

if __name__ == '__main__':
    unittest.main()
