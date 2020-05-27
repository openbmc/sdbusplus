#!/usr/bin/env python3
from setuptools import setup, find_packages

setup(
    name="sdbusplus",
    version="1.0",
    packages=find_packages(),
    install_requires=["inflection", "mako", "pyyaml"],
    scripts=["sdbus++", "sdbus++-gendir"],
    package_data={"sdbusplus": ["templates/*.mako"]},
    url="http://github.com/openbmc/sdbusplus",
    classifiers=["License :: OSI Approved :: Apache Software License"],
)
