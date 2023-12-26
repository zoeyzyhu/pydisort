#! python3
""" Test isotropic scattering with pydisort."""
# pylint: disable = no-name-in-module, invalid-name, import-error
import unittest
from numpy import array, pi
from numpy.testing import assert_allclose
from pydisort import disort, get_phase_function, RFLDIR, FLDN, FLUP


# cdisort test01
class PyDisortTests(unittest.TestCase):
    """Test unit: isotropic scattering with pydisort."""

    def setUp(self):
        """Set up the test."""
        self.flags = {
                "ibcnd": 0,
                "usrtau": 1,
                "usrang": 1,
                "lamber": 1,
                "planck": 0,
                "spher": 0,
                "onlyfl": 0,
                "quiet": 1,
                "intensity_correction": 1,
                "old_intensity_correction": 1,
                "general_source": 0,
                "output_uum": 0,
                "print-input": 1,
                "print-fluxes": 0, 
                "print-intensity": 0,
                "print-transmissivity": 0,
                "print-phase-function": 1,
                }

    def test_isotropic_scattering(self):
        """Test isotropic scattering."""
        ds = disort().set_flags(self.flags)
        ds.set_header("01. test isotropic scattering")

        # set dimension
        ds.set_atmosphere_dimension(nlyr=1, nstr=16, nmom=16
         ).set_intensity_dimension(nuphi=1, nutau=2, numu=6
         ).seal()

        # get scattering moments
        _, _, nmom = ds.dimensions()
        pmom = get_phase_function(nmom, "isotropic")

        # set boundary conditions
        ds.umu0 = 0.1
        ds.phi0 = 0.0
        ds.albedo = 0.0
        ds.fluor = 0.0

        # set output optical depth and polar angles
        umu = array([-1.0, -0.5, -0.1, 0.1, 0.5, 1.0])
        uphi = array([0.0])
        utau = array([0.0, 0.03125])

        # case No.1
        print("==== Case No.1 ====")
        ds.fbeam = pi / ds.umu0
        ds.fisot = 0.0

        ds.set_optical_thickness(array([utau[-1]]))
        ds.set_single_scattering_albedo(array([0.2]))
        ds.set_phase_moments(pmom)
        ds.set_user_optical_depth(utau)
        ds.set_user_cosine_polar_angle(umu)
        ds.set_user_azimuthal_angle(uphi)

        rad, flx = ds.run()

        self.assertEqual(rad.shape, (1, 2, 6))
        assert_allclose(
            rad,
            array(
                [
                    [
                        [0.0, 0.0, 0.0, 0.11777066, 0.02641704, 0.01340413],
                        [0.01338263, 0.02633235, 0.11589789, 0.0, 0.0, 0.0],
                    ]
                ]
            ),
            atol=1e-8,
            rtol=1e-5,
        )

        flx = flx[:, [RFLDIR, FLDN, FLUP]]
        assert_allclose(
            flx,
            array(
                [
                    [3.14159265e00, 0., 7.99450975e-02],
                    [2.29843829e00, 7.94107954e-02, 0.],
                ]
            ),
            atol=1e-8,
            rtol=1e-5,
        )

        # case No.2
        print("==== Case No.2 ====")
        ds.set_single_scattering_albedo(array([1.0]))
        rad, flx = ds.run()
        assert_allclose(
            rad,
            array(
                [
                    [
                        [0.0, 0.0, 0.0, 0.62288378, 0.13976294, 0.07091916],
                        [0.07081093, 0.1393367, 0.61345786, 0.0, 0.0, 0.0],
                    ]
                ]
            ),
            atol=1e-8,
            rtol=1e-5,
        )

        flx = flx[:, [RFLDIR, FLDN, FLUP]]
        assert_allclose(
            flx,
            array(
                [
                    [3.14159265e00, 0., 4.22921778e-01],
                    [2.29843829e00, 4.20232590e-01, 0.],
                ]
            ),
            atol=1e-8,
            rtol=1e-5,
        )


if __name__ == "__main__":
    unittest.main()
