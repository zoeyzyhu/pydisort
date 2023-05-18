import pydisort

# Create an instance of DisortWrapper
pydt = pydisort.disort.from_file("input.toml")

# Set the atmosphere dimensions
pydt.set_atmosphere_dimension(10, 20, 30, 40)

# Set the flags
flags = {
    "ibcnd": True,
    "usrtau": False,
    "usrang": True,
    "lamber": False,
    "planck": True,
    "spher": False,
    "onlyfl": True,
    "quiet": False,
    "intensity_correction": True,
    "old_intensity_correction": False,
    "general_source": True,
    "output_uum": False
}
pydt.set_flags(flags)

# Set the intensity dimensions
pydt.set_intensity_dimension(5, 15, 25)

# Set other parameters
# pydt.btemp = 300.0
# pydt.ttemp = 280.0
# pydt.fluor = 0.5
# pydt.albedo = 0.2
# pydt.fisot = 0.8
# pydt.fbeam = 0.6
# pydt.temis = 290.0
# pydt.umu0 = 0.8
# pydt.phi0 = 0.0

# Finalize the DisortWrapper instance
# pydt.finalize()

# Run RT Flux calculation
flxup, flxdn = pydt.run_rt_flux()
print("RT Flux Up:", flxup)
print("RT Flux Down:", flxdn)

# Run RT Intensity calculation
rt_intensity = pydt.run_rt_intensity()
print("RT Intensity:", rt_intensity)

