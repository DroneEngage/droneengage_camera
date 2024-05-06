#!/bin/bash

pushd src

python build/linux/sysroot_scripts/install-sysroot.py --arch=x86


gn gen out/x64_4606 --args='is_debug=true  rtc_include_tests=false treat_warnings_as_errors=false use_rtti=true is_component_build=false  enable_iterator_debugging=false   is_clang=false use_sysroot=false linux_use_bundled_binutils=false use_custom_libcxx=false use_custom_libcxx_for_host=false target_os="linux"  target_cpu="x64"'

popd