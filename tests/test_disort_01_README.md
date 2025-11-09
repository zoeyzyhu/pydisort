# Test Documentation: test_disort_01.py

## Overview

This test suite provides Python equivalents of `disort_test01()` from the C implementation
(`tests/cdisort213/test_cdisort.c`). It contains 6 comprehensive test cases that validate
the DISORT radiative transfer model for isotropic scattering scenarios.

## Test Structure

All tests follow a common structure:
1. **Setup**: Configure DISORT options with specific flags and parameters
2. **Boundary Conditions**: Define incident radiation (beam or isotropic)
3. **Medium Properties**: Set optical depth and single-scatter albedo
4. **Execution**: Run the forward model
5. **Validation**: Compare results against reference values

## Test Cases

### Test Case 1: Thin Absorbing Medium with Beam Source
- **Optical Depth**: τ = 0.03125 (thin)
- **Single-Scatter Albedo**: ω = 0.2 (strong absorption)
- **Source**: Directional beam (µ₀ = 0.1)
- **Physics**: Most photons are absorbed; minimal scattering contribution
- **Key Insight**: Tests basic absorption-dominated regime

### Test Case 2: Thin Conservative Scattering with Beam Source
- **Optical Depth**: τ = 0.03125 (thin)
- **Single-Scatter Albedo**: ω = 1.0 (conservative)
- **Source**: Directional beam (µ₀ = 0.1)
- **Physics**: No absorption; all energy redistributed by scattering
- **Key Insight**: Tests energy conservation in thin medium

### Test Case 3: Thin Medium with Isotropic Source
- **Optical Depth**: τ = 0.03125 (thin)
- **Single-Scatter Albedo**: ω = 0.99 (near-conservative)
- **Source**: Isotropic incident radiation (uniform from above)
- **Physics**: Diffuse illumination with minimal absorption
- **Key Insight**: Tests response to non-directional radiation field

### Test Case 4: Thick Absorbing Medium with Beam Source
- **Optical Depth**: τ = 32 (optically thick)
- **Single-Scatter Albedo**: ω = 0.2 (strong absorption)
- **Source**: Directional beam (µ₀ = 0.1)
- **Physics**: Most radiation absorbed; very little penetration to bottom
- **Key Insight**: Tests deep absorption in optically thick atmosphere

### Test Case 5: Thick Conservative Scattering with Beam Source
- **Optical Depth**: τ = 32 (optically thick)
- **Single-Scatter Albedo**: ω = 1.0 (conservative)
- **Source**: Directional beam (µ₀ = 0.1)
- **Physics**: Multiple scattering events; diffusion-dominated transport
- **Key Insight**: Tests multiple scattering in thick medium without absorption

### Test Case 6: Thick Medium with Isotropic Source
- **Optical Depth**: τ = 32 (optically thick)
- **Single-Scatter Albedo**: ω = 0.99 (near-conservative)
- **Source**: Isotropic incident radiation
- **Physics**: Complex interplay of diffusion and weak absorption
- **Key Insight**: Tests most general case with thick medium and diffuse source

## Output Quantities Validated

Each test validates three types of outputs:

### 1. Upward and Downward Fluxes
- Shape: `(1, 1, 2, 2)` → `(2, 2)` after squeezing
- Dimensions: [tau_level, flux_component]
- Components:
  - `[0, 0]`: Upward flux at top (τ = 0)
  - `[0, 1]`: Downward flux at top (τ = 0)
  - `[1, 0]`: Upward flux at bottom (τ = τ_max)
  - `[1, 1]`: Downward flux at bottom (τ = τ_max)

### 2. Detailed Flux Outputs
- Shape: `(1, 1, 2, 8)` → `(2, 8)` after squeezing
- Dimensions: [tau_level, flux_type]
- Flux types (8 components per level):
  0. Direct beam flux (downward)
  1. Diffuse downward flux
  2. Diffuse upward flux
  3. Mean intensity derivative (heating rate)
  4. Net flux
  5. Net flux (alternative calculation)
  6. Radiative flux divergence
  7. Actinic flux

### 3. Radiance Intensities
- Shape: `(1, 1, 1, 2, 6)` → `(2, 6)` after squeezing
- Dimensions: [tau_level, mu_direction]
- 6 polar angle directions:
  - µ = -1.0, -0.5, -0.1 (downward directions)
  - µ = 0.1, 0.5, 1.0 (upward directions)

## Physical Interpretation

### Optical Depth (τ)
- **τ = 0.03125**: Thin medium, most radiation passes through
- **τ = 32**: Thick medium, strong attenuation of direct beam

### Single-Scatter Albedo (ω)
- **ω = 0.2**: Strong absorption (20% scattered, 80% absorbed)
- **ω = 0.99**: Weak absorption (99% scattered, 1% absorbed)
- **ω = 1.0**: Conservative scattering (100% scattered, no absorption)

### Source Types
- **Beam source** (`fbeam`): Directional incident radiation at angle µ₀
- **Isotropic source** (`fisot`): Uniform diffuse radiation from above

## Test Configuration

All tests use:
- **Number of streams** (`nstr`): 16 (8 upward, 8 downward)
- **Number of moments** (`nmom`): 16 (sufficient for isotropic scattering)
- **Phase function**: Isotropic (uniform scattering in all directions)
- **User optical depths**: 2 levels (top and bottom)
- **User polar angles**: 6 directions (3 up, 3 down)
- **User azimuthal angles**: 1 (azimuthally symmetric)

## Reference

These tests are based on:
- Van de Hulst, H.C., 1980: *Multiple Light Scattering, Tables, Formulas and Applications*,
  Volumes 1 and 2, Academic Press, New York (VH1, Table 12).
- Original DISORT test suite by Stamnes et al.

## Running the Tests

```bash
# Run all tests
pytest tests/test_disort_01.py -v

# Run specific test
pytest tests/test_disort_01.py::test_case1 -v

# Run with detailed output
pytest tests/test_disort_01.py -v -s
```

## Expected Behavior

All tests should pass with:
- **Relative tolerance**: 1e-4 to 1e-5
- **Absolute tolerance**: 1e-4 to 1e-8

These tolerances account for:
- Numerical precision differences between C and Python implementations
- Floating-point arithmetic variations
- Iterative solver convergence criteria

## Troubleshooting

If tests fail:
1. Verify `pydisort` is installed: `pip install pydisort`
2. Check PyTorch version compatibility: `torch>=2.7.0,<=2.7.1`
3. Ensure NumPy is compatible with PyTorch
4. Review test output for specific assertion failures
5. Compare actual vs. expected values to identify discrepancies
