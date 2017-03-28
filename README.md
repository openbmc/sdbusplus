# sdbusplus

sdbusplus is a library and a tool for generating C++ bindings to dbus.

## How to use tools/sdbus++

The path of your file will be the interface name. For example, for an interface
`xyz.openbmc_project.control.Chassis`, you would create the following file:
`xyz/openbmc_project/control/Chassis.interface.yaml`. Similary, for errors, you
would create `xyz/openbmc_project/control/Chassis.errors.yaml`.

Generating all the files:
```
root_dir=$(readlink -f ../phosphor-dbus-interfaces)
desired_interface=xyz.openbmc_project.control.Chassis
file_prefix=chassis_interface
file_exp_prefix=chassis_interface_exceptions
outdir=../phosphor-chassis-control/generated

# Server bindings
python tools/sdbus++ --templatedir=tools/sdbusplus/templates --rootdir=$root_dir interface server-header $desired_interface > $outdir/$file_prefix.hpp
python tools/sdbus++ --templatedir=tools/sdbusplus/templates --rootdir=$root_dir interface server-cpp $desired_interface > $outdir/$file_prefix.cpp

# Exception bindings
python tools/sdbus++ --templatedir=tools/sdbusplus/templates --rootdir=$root_dir error exception-header $desired_interface > $outdir/$file_exp_prefix.hpp
python tools/sdbus++ --templatedir=tools/sdbusplus/templates --rootdir=$root_dir error exception-cpp $desired_interface > $outdir/$file_exp_prefix.cpp

# Docs
python tools/sdbus++ --templatedir=tools/sdbusplus/templates --rootdir=$root_dir interface markdown $desired_interface > $outdir/$file_prefix.md
python tools/sdbus++ --templatedir=tools/sdbusplus/templates --rootdir=$root_dir error markdown $desired_interface > $outdir/$file_exp_prefix.md
```

