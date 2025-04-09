# CMakeList.txt : projet CMake pour projettut, incluez la source et définissez
# la logique spécifique au projet ici.
#
cmake_minimum_required (VERSION 3.8)

# Activez Rechargement à chaud pour les compilateurs MSVC si cela est pris en charge.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# TODO: Ajoutez des tests et installez des cibles si nécessaire.


cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
include(D:/vcpkg/scripts/buildsystems/vcpkg.cmake)
project(SimplePnP)
find_package(OpenCV REQUIRED)

add_executable (SimplePnP main.cpp)

target_link_libraries (SimplePnP  opencv)


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET SimplePnP PROPERTY CXX_STANDARD 20)
endif()
