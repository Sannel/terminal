# CMake generated Testfile for 
# Source directory: /home/adam/source/Sannel/terminal
# Build directory: /home/adam/source/Sannel/terminal/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[appstreamtest]=] "/usr/bin/cmake" "-DAPPSTREAMCLI=/usr/bin/appstreamcli" "-DINSTALL_FILES=/home/adam/source/Sannel/terminal/build/install_manifest.txt" "-P" "/usr/share/ECM/kde-modules/appstreamtest.cmake")
set_tests_properties([=[appstreamtest]=] PROPERTIES  _BACKTRACE_TRIPLES "/usr/share/ECM/kde-modules/KDECMakeSettings.cmake;168;add_test;/usr/share/ECM/kde-modules/KDECMakeSettings.cmake;187;appstreamtest;/usr/share/ECM/kde-modules/KDECMakeSettings.cmake;0;;/home/adam/source/Sannel/terminal/CMakeLists.txt;16;include;/home/adam/source/Sannel/terminal/CMakeLists.txt;0;")
subdirs("lib/kterm-vt")
subdirs("lib/kterm-pty")
subdirs("lib/kterm-core")
subdirs("lib/kterm-settings")
subdirs("lib/kterm-widget")
subdirs("kterm")
