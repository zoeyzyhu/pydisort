#! python3
""" This script is part of the pydisort package and is used to load preset files.

This module contains the following functions:
    * load_preset - loads a preset file and returns a disort object.
    * print_available_presets - prints the names of available preset files.
"""
# pylint: disable = no-name-in-module, invalid-name, import-error

from pydisort import disort

PRESETS = [
    "scattering",
    "jupiter_infrared",
    "earth_infrared"
]


def load_preset(name: str) -> disort:
    """ Loads a preset file and returns a disort object.

    Parameters:
        name (str): The name of the preset file to be loaded.

    Returns:
        disort: The disort object loaded from the preset file.

    Raises:
        ValueError: If the name provided is not one of the available presets.
    """
    if name in PRESETS:
        return disort.from_file(f"{name}.toml")

    raise ValueError(
        f"Unknown preset: '{name}'. Available presets are: {', '.join(PRESETS)}")


def print_available_presets():
    """ Prints the names of the available preset files. """
    print('Available presets are:')
    for preset in PRESETS:
        print(preset)


def test():
    """ Executes a simple test of the script functionality. """
    print_available_presets()
    try:
        disort_obj = load_preset(input("Enter preset name: "))
        print(disort_obj)
    except ValueError as error:
        print(f"An error occurred: {error}")


if __name__ == '__main__':
    test()
