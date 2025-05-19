#!/bin/bash

pushd src

python build/linux/sysroot_scripts/install-sysroot.py --arch=arm64

gn gen out/rpi-5 --args='is_debug=true use_system_libjpeg=true treat_warnings_as_errors=false  rtc_include_tests=false rtc_use_h264=true  rtc_build_examples=true rtc_build_tools=true rtc_exclude_audio_processing_module=true rtc_use_dummy_audio_file_devices=true target_cpu="arm64" is_clang=true  '
popd   
