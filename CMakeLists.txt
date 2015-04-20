project(FWMFW C CXX)
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

if (NOT WIN32)
    message(FATAL_ERROR "Only Windows platform is supported.")
endif ()

set(
    SRCS
    Source/FWMFW.cxx
	Source/Utils.cxx
	Source/Utils.hxx
    Source/WinNetFW.cxx
    Source/WinNetFW.hxx
)

add_executable(FWMFW ${SRCS})
