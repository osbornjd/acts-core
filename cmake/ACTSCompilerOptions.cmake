# set ACTS compiler flags
set (ACTS_CXX_FLAGS "-std=c++14 -Wall ${CMAKE_CXX_FLAGS}")
set (ACTS_CXX_FLAGS_DEBUG "--coverage ${CMAKE_CXX_FLAGS_DEBUG}")
set (ACTS_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
set (ACTS_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
set (ACTS_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

# set ACTS linker flags
set (ACTS_EXE_LINKER_FLAGS_DEBUG "--coverage ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
set (ACTS_SHARED_LINKER_FLAGS_DEBUG "--coverage ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")

# assign to CXX flags
set (CMAKE_CXX_FLAGS "${ACTS_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS_DEBUG "${ACTS_CXX_FLAGS_DEBUG}")
set (CMAKE_CXX_FLAGS_MINSIZEREL "${ACTS_CXX_FLAGS_MINSIZEREL}")
set (CMAKE_CXX_FLAGS_RELEASE "${ACTS_CXX_FLAGS_RELEASE}")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${ACTS_CXX_FLAGS_RELWITHDEBINFO}")

# assign to linker flags
set (CMAKE_EXE_LINKER_FLAGS_DEBUG "${ACTS_EXE_LINKER_FLAGS_DEBUG}")
set (CMAKE_SHARED_LINKER_FLAGS_DEBUG "${ACTS_SHARED_LINKER_FLAGS_DEBUG}")

# silence warning about missing RPATH on Mac OSX
set (CMAKE_MACOSX_RPATH 1)

# Use CCache if available
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  message(STATUS "Use ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)
