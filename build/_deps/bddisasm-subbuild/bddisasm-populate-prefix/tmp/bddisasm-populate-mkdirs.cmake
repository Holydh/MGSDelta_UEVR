# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/bddisasm-src"
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/bddisasm-build"
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/bddisasm-subbuild/bddisasm-populate-prefix"
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/bddisasm-subbuild/bddisasm-populate-prefix/tmp"
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/bddisasm-subbuild/bddisasm-populate-prefix/src/bddisasm-populate-stamp"
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/bddisasm-subbuild/bddisasm-populate-prefix/src"
  "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/bddisasm-subbuild/bddisasm-populate-prefix/src/bddisasm-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/bddisasm-subbuild/bddisasm-populate-prefix/src/bddisasm-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Dam/Dev/Cpp/UEVR_BaseCPPproject/MGSDelta_UEVR/build/_deps/bddisasm-subbuild/bddisasm-populate-prefix/src/bddisasm-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
