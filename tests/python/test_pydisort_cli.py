import unittest

# from rules_python.python.runfiles import runfiles

# gazelle:ignore wrapper
import pydisort
from numpy import array

class TestPydisort(unittest.TestCase):

    def setUp(self):
        self.toml_path = "input.toml"
        assert self.toml_path, '{self.toml_path} is not found.'

    def test_from_file(self):
        disort = pydisort.disort.from_file(self.toml_path)

    def test_set_optical_depth(self):
        disort = pydisort.disort.from_file(self.toml_path)
        tau = [0.1, 0.2, 0.3, 0.4, 0.5]
        disort.set_optical_depth(tau)

    def test_set_single_scattering_albedo(self):
        disort = pydisort.disort.from_file(self.toml_path)
        g = array([0.1, 0.2, 0.3, 0.4, 0.5])
        disort.set_single_scattering_albedo(g)

    def test_set_level_temperature(self):
        disort = pydisort.disort.from_file(self.toml_path)
        temp = array([100., 100., 200., 300., 400., 500.])
        disort.set_level_temperature(temp)

    def test_set_wavenumber_range(self):
        disort = pydisort.disort.from_file(self.toml_path)
        disort.set_wavenumber_range_invcm(10., 100.)

    def test_run_rt_flux(self):
        disort = pydisort.disort.from_file(self.toml_path) \
            .set_optical_depth([0.1, 0.2, 0.3, 0.4, 0.5])   \
            .set_level_temperature([100., 100., 200., 300., 400., 500.])    \
            .set_single_scattering_albedo([0.1, 0.2, 0.3, 0.4, 0.5])    \
            .set_wavenumber_range_invcm(10., 100.)
        flxup, flxdn = disort.run_rt_flux()
        print(flxup, flxdn)

    def test_run_rt_flux_dict(self):
        disort = pydisort.disort.from_file(self.toml_path)
        temp = array([100., 100., 200., 300., 400., 500.])
        tau = array([0.1, 0.2, 0.3, 0.4, 0.5])
        ssa = array([0.1, 0.2, 0.3, 0.4, 0.5])
        flxup, flxdn = disort.run_rt_flux({'temp':temp, 'tau':tau, 'ssa':ssa})

    def test_run_rt_intensity(self):
        disort = pydisort.disort.from_file(self.toml_path)
        temp = array([100., 100., 200., 300., 400., 500.])
        tau = array([0.1, 0.2, 0.3, 0.4, 0.5])
        ssa = array([0.1, 0.2, 0.3, 0.4, 0.5])
        uu = disort.run_rt_intensity({'temp':temp, 'tau':tau, 'ssa':ssa})
        print(uu.shape)

if __name__ == '__main__':
    unittest.main()
