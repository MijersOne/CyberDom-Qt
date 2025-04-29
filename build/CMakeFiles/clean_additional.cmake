# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\CyberDom_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\CyberDom_autogen.dir\\ParseCache.txt"
  "CyberDom_autogen"
  )
endif()
