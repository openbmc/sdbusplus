# sdbusplus

sdbusplus is a library and a tool for generating C++ bindings to dbus.

## How to use tools/sdbus++

The path of your file will be the interface name. For example, for an interface
`xyz.openbmc_project.control.Chassis`, you would create the following file:
`xyz/openbmc_project/control/Chassis.interface.yaml`.


Generating all the files:
```
root_dir=$(readlink -f ../phosphor-dbus-interfaces)
desired_interface=xyz.openbmc_project.control.Chassis
file_prefix=chassis_interface
outdir=../phosphor-chassis-control/generated

python tools/sdbus++ --templatedir=tools/sdbusplus/templates --rootdir=$root_dir interface server-header $desired_interface > $outdir/$file_prefix.hpp
python tools/sdbus++ --templatedir=tools/sdbusplus/templates --rootdir=$root_dir interface server-cpp $desired_interface > $outdir/$file_prefix.cpp
python tools/sdbus++ --templatedir=tools/sdbusplus/templates --rootdir=$root_dir interface markdown $desired_interface > $outdir/$file_prefix.md
```
