#!/usr/bin/env python3
from setuptools import find_packages, setup

setup(
    name="sdbusplus",
    version="1.0",
    packages=find_packages(),
    install_requires=["inflection", "mako", "pyyaml"],
    scripts=["sdbus++", "sdbus++-gen-meson"],
    package_data={"sdbusplus": ["schemas/*.yaml", "templates/*.mako"]},
    url="http://github.com/openbmc/sdbusplus",
    classifiers=["License :: OSI Approved :: Apache Software License"],
)
