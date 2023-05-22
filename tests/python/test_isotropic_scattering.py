#! python3
from numpy import array, pi
from pydisort import disort, get_legendre_coefficients
from numpy.testing import assert_allclose
import os, unittest

# cdisort test01
class PyDisortTests(unittest.TestCase):
    def setUp(self):
        self.toml_path = "isotropic_scattering.toml"
        assert os.path.exists(self.toml_path), f"{self.toml_path} does not exist."
    
    def test_isotropic_scattering(self):
        ds = disort.from_file(self.toml_path)
        ds.set_header("01. test isotropic scattering")

        # set dimension
        ds.set_atmosphere_dimension(
            nlyr = 1, nstr = 16, nmom = 16, nphase = 16)
        ds.set_intensity_dimension(
            nphi = 1, numu = 6, ntau = 2)
        ds.finalize()

        # get scattering moments
        pmom = get_legendre_coefficients(ds.get_nmom(), "isotropic")

        # set boundary conditions
        ds.umu0 = 0.1
        ds.phi0 = 0.
        ds.albedo = 0.
        ds.fluor = 0.

        # set output optical depth and polar angles
        umu = array([-1., -0.5, -0.1, 0.1, 0.5, 1.0])
        uphi = array([0.])

        # case No.1
        ds.fbeam = pi/ds.umu0
        ds.fisot = 0.
        utau = array([0., 0.03125])
        ssa = array([0.2])

        tau = array([utau[-1]])
        result = ds.run_rt_intensity({
            'tau':tau, 'ssa':ssa, 'pmom':pmom,
            'utau':utau, 'umu':umu, 'uphi':uphi
            })

        self.assertEqual(result.shape, (1, 2, 6))
        assert_allclose(result, array([
            [[0., 0., 0., 0.11777066, 0.02641704, 0.01340413],
             [0.01338263, 0.02633235, 0.11589789, 0., 0., 0.]]
            ]), atol=1e-8, rtol=1e-5)

if __name__ == '__main__':
    unittest.main()
