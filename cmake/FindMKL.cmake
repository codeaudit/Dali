# - Find the MKL libraries
# from the beautiful land of http://sourceforge.net/p/gadgetron/gadgetron/ci/master/tree/cmake/FindMKL.cmake
# and also: https://github.com/Eyescale/CMake/blob/master/FindMKL.cmake
# Modified from Armadillo's ARMA_FindMKL.cmake
# This module defines
#  MKL_INCLUDE_DIR, the directory for the MKL headers
#  MKL_LIB_DIR, the directory for the MKL library files
#  MKL_COMPILER_LIB_DIR, the directory for the MKL compiler library files
#  MKL_LIBRARIES, the libraries needed to use Intel's implementation of BLAS & LAPACK.
#  MKL_FOUND, If false, do not try to use MKL; if true, the macro definition USE_MKL is added.

# Set the include path
# TODO: what if MKL is not installed in /opt/intel/mkl?
# try to find at /opt/intel/mkl
# in windows, try to find MKL at C:/Program Files (x86)/Intel/Composer XE/mkl

if ( WIN32 )
  if(NOT DEFINED ENV{MKLROOT_PATH})
    set(MKLROOT_PATH "C:/Program Files (x86)/Intel/Composer XE" CACHE PATH "Where the MKL are stored")
  endif(NOT DEFINED ENV{MKLROOT_PATH})
else ( WIN32 )
    set(MKLROOT_PATH "/opt/intel" CACHE PATH "Where the MKL are stored")
endif ( WIN32 )

if (EXISTS ${MKLROOT_PATH}/mkl)
    SET(MKL_FOUND TRUE)
    message("MKL is found at ${MKLROOT_PATH}/mkl")
    IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set( USE_MKL_64BIT On )
        if ( ARMADILLO_FOUND )
            if ( ARMADILLO_BLAS_LONG_LONG )
                set( USE_MKL_64BIT_LIB On )
                ADD_DEFINITIONS(-DMKL_ILP64)
                message("MKL is linked against ILP64 interface ... ")
            endif ( ARMADILLO_BLAS_LONG_LONG )
        endif ( ARMADILLO_FOUND )
    ELSE(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set( USE_MKL_64BIT Off )
    ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 8)
else (EXISTS ${MKLROOT_PATH}/mkl)
    SET(MKL_FOUND FALSE)
    message("MKL is NOT found ... ")
endif (EXISTS ${MKLROOT_PATH}/mkl)

if (MKL_FOUND)
    set(MKL_INCLUDE_DIR "${MKLROOT_PATH}/mkl/include")
    ADD_DEFINITIONS(-DUSE_MKL)
    if ( USE_MKL_64BIT )
        set(MKL_LIB_DIR "${MKLROOT_PATH}/mkl/lib")
        if (EXISTS ${MKLROOT_PATH}/compiler})
            set(MKL_COMPILER_LIB_DIR "${MKLROOT_PATH}/compiler/lib/intel64")
        else (EXISTS ${MKLROOT_PATH}/compiler})
            if (EXISTS ${MKLROOT_PATH}/composer_xe_2015.1.108/compiler)
                message("Inside the Armadillo's belly")
                set(MKL_COMPILER_LIB_DIR "${MKLROOT_PATH}/composer_xe_2015.1.108/compiler/lib")
            endif (EXISTS ${MKLROOT_PATH}/composer_xe_2015.1.108/compiler)
        endif (EXISTS ${MKLROOT_PATH}/compiler})
        set(MKL_COMPILER_LIB_DIR ${MKL_COMPILER_LIB_DIR} "${MKLROOT_PATH}/lib")
        if ( USE_MKL_64BIT_LIB )
                if (WIN32)
                    set(MKL_LIBRARIES ${MKL_LIBRARIES} mkl_intel_ilp64)
                else (WIN32)
                    set(MKL_LIBRARIES ${MKL_LIBRARIES} mkl_intel_ilp64)
                endif (WIN32)
        else ( USE_MKL_64BIT_LIB )
            find_library(MKL_LP64_LIBRARY
              mkl_intel_lp64
              PATHS
                ${MKL_LIB_DIR}
                ${MKL_COMPILER_LIB_DIR}
            )
            set(MKL_LIBRARIES ${MKL_LIBRARIES} ${MKL_LP64_LIBRARY})
        endif ( USE_MKL_64BIT_LIB )
    else ( USE_MKL_64BIT )
        set(MKL_LIB_DIR "${MKLROOT_PATH}/mkl/lib/ia32")
        set(MKL_COMPILER_LIB_DIR "${MKLROOT_PATH}/compiler/lib/ia32")
        set(MKL_COMPILER_LIB_DIR ${MKL_COMPILER_LIB_DIR} "${MKLROOT_PATH}/lib/ia32")
        if ( WIN32 )
            set(MKL_LIBRARIES ${MKL_LIBRARIES} mkl_intel_c)
        else ( WIN32 )
            set(MKL_LIBRARIES ${MKL_LIBRARIES} mkl_intel)
        endif ( WIN32 )
    endif ( USE_MKL_64BIT )

    if (BLA_VENDOR STREQUAL "Intel10_32" OR BLA_VENDOR STREQUAL "All")
        find_library(MKL_INTELTHREAD_LIBRARY
          mkl_intel_thread
          PATHS
            ${MKL_LIB_DIR}
            ${MKL_COMPILER_LIB_DIR}
        )
        find_library(MKL_CORE_LIBRARY
          mkl_core
          PATHS
            ${MKL_LIB_DIR}
            ${MKL_COMPILER_LIB_DIR}
        )
        find_library(MKL_IOMP5_LIBRARY
          iomp5
          PATHS
            ${MKL_LIB_DIR}
            ${MKL_COMPILER_LIB_DIR}
        )
        SET(MKL_LIBRARIES ${MKL_LIBRARIES} ${MKL_INTELTHREAD_LIBRARY})
        SET(MKL_LIBRARIES ${MKL_LIBRARIES} ${MKL_CORE_LIBRARY})
        SET(MKL_LIBRARIES ${MKL_LIBRARIES} ${MKL_IOMP5_LIBRARY})
    else (BLA_VENDOR STREQUAL "Intel10_32" OR BLA_VENDOR STREQUAL "All")
        SET(MKL_LIBRARIES ${MKL_LIBRARIES} mkl_gnu_thread)
        SET(MKL_LIBRARIES ${MKL_LIBRARIES} mkl_core)
    endif (BLA_VENDOR STREQUAL "Intel10_32" OR BLA_VENDOR STREQUAL "All")
endif (MKL_FOUND)

IF (MKL_FOUND)
    IF (NOT MKL_FIND_QUIETLY)
        MESSAGE(STATUS "Found MKL libraries: ${MKL_LIBRARIES}")
        MESSAGE(STATUS "MKL_INCLUDE_DIR: ${MKL_INCLUDE_DIR}")
        MESSAGE(STATUS "MKL_LIB_DIR: ${MKL_LIB_DIR}")
        MESSAGE(STATUS "MKL_COMPILER_LIB_DIR: ${MKL_COMPILER_LIB_DIR}")
    ENDIF (NOT MKL_FIND_QUIETLY)

    INCLUDE_DIRECTORIES( ${MKL_INCLUDE_DIR} )
    LINK_DIRECTORIES( ${MKL_LIB_DIR} ${MKL_COMPILER_LIB_DIR} )
ELSE (MKL_FOUND)
    IF (MKL_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find MKL libraries")
    ENDIF (MKL_FIND_REQUIRED)
ENDIF (MKL_FOUND)

# MARK_AS_ADVANCED(MKL_LIBRARY)