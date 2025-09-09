# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/kananlib-src"
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/kananlib-build"
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/kananlib-subbuild/kananlib-populate-prefix"
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/kananlib-subbuild/kananlib-populate-prefix/tmp"
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/kananlib-subbuild/kananlib-populate-prefix/src/kananlib-populate-stamp"
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/kananlib-subbuild/kananlib-populate-prefix/src"
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/kananlib-subbuild/kananlib-populate-prefix/src/kananlib-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/kananlib-subbuild/kananlib-populate-prefix/src/kananlib-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/kananlib-subbuild/kananlib-populate-prefix/src/kananlib-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
