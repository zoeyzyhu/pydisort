#! python3    test_Rayleigh
from numpy import array, pi
from pydisort import disort, get_legendre_coefficients, Radiant
from numpy.testing import assert_allclose
import os, unittest


# cdisort test01
class PyDisortTests(unittest.TestCase):
    def setUp(self):
        self.toml_path = "Rayleigh_scattering.toml"
        assert os.path.exists(self.toml_path), f"{self.toml_path} does not exist."

    def test_Rayleigh_scattering(self):
        # creating an instance of the disort class from a toml file.
        ds = disort.from_file(self.toml_path)

        # set the header of the disort class
        ds.set_header("02. test Rayleigh scattering")

        # set dimension
        ds.set_atmosphere_dimension(
            nlyr=1, nstr=16, nmom=16, nphase=16
        ).set_intensity_dimension(nuphi=1, nutau=2, numu=6).finalize()

        # get scattering moments
        pmom = get_legendre_coefficients(ds.get_nmom(), "rayleigh")

        # set boundary conditions
        ds.umu0 = 0.080442
        ds.phi0 = 0.0
        ds.albedo = 0.0
        ds.fluor = 0.0
        ds.fbeam = pi
        ds.fisot = 0.0

        # set output polar angles and initialize optical depth
        umu = array([-0.981986, -0.538263, -0.018014, 0.018014, 0.538263, 0.981986])
        uphi = array([0.0])
        utau = array([0.0, 0.0])

        # set optical depth at the bottom of layer and single scattering albedo for the 4 cases
        btau_list = [0.2, 0.2, 5.0, 5.0]
        ssa_list = [0.5, 1.0, 0.5, 1.0]

        ans_intensity = {}
        ans_flux = {}
        # correct answers for case 1
        ans_intensity[0] = array(
            [
                [
                    [0.0, 0.0, 0.0, 0.161796, 0.0211501, 0.00786713],
                    [0.00771897, 0.0200778, 0.0257685, 0.0, 0.0, 0.0],
                ]
            ]
        )
        ans_flux[0] = array(
            [
                [2.52716e-01, 0.0, 5.35063e-02],
                [2.10311e-02, 4.41791e-02, 0.0],
            ]
        )

        # correct answers for case 2
        ans_intensity[1] = array(
            [
                [
                    [0.0, 0.0, 0.0, 3.47678e-01, 4.87120e-02, 1.89387e-02],
                    [1.86027e-02, 4.64061e-02, 6.77603e-02, 0.0, 0.0, 0.0],
                ]
            ]
        )
        ans_flux[1] = array(
            [
                [2.52716e-01, 0.0, 1.25561e-01],
                [2.10311e-02, 1.06123e-01, 0.0],
            ]
        )

        # correct answers for case 3
        ans_intensity[2] = array(
            [
                [
                    [0.0, 0.0, 0.0, 1.62566e-01, 2.45786e-02, 1.01498e-02],
                    [1.70004e-04, 3.97168e-05, 1.32472e-05, 0.0, 0.0, 0.0],
                ]
            ]
        )
        ans_flux[2] = array(
            [
                [2.52716e-01, 0.0, 6.24730e-02],
                [2.56077e-28, 2.51683e-04, 0.0],
            ]
        )

        # correct answers for case 4
        ans_intensity[3] = array(
            [
                [
                    [0.0, 0.0, 0.0, 3.64010e-01, 8.26993e-02, 4.92370e-02],
                    [1.05950e-02, 7.69337e-03, 3.79276e-03, 0.0, 0.0, 0.0],
                ]
            ]
        )
        ans_flux[3] = array(
            [
                [2.52716e-01, 0.0, 2.25915e-01],
                [0.0, 2.68008e-02, 0.0],
            ]
        )

        for icas in range(4):
            print("\n===== Case No.%d =====" % (icas + 1))
            utau[1] = btau_list[icas]
            tau = array([utau[-1]])
            ssa = array([ssa_list[icas]])

            # calculate intensity
            result = ds.run_with(
                {
                    "tau": tau,
                    "ssa": ssa,
                    "pmom": pmom,
                    "utau": utau,
                    "umu": umu,
                    "uphi": uphi,
                }
            ).get_intensity()
            # verify intensity
            self.assertEqual(result.shape, (1, 2, 6))
            assert_allclose(
                result,
                ans_intensity[icas],
                atol=1e-8,
                rtol=1e-5,
            )

            # calculate flux
            result = ds.get_flux()[:, [Radiant.RFLDIR, Radiant.FLDN, Radiant.FLUP]]
            # verify flux
            assert_allclose(
                result,
                ans_flux[icas],
                atol=1e-8,
                rtol=1e-5,
            )


if __name__ == "__main__":
    unittest.main()
