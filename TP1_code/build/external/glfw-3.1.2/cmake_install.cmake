# Install script for directory: /mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/external/glfw-3.1.2

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE DIRECTORY FILES "/mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/external/glfw-3.1.2/include/GLFW" FILES_MATCHING REGEX "/glfw3\\.h$" REGEX "/glfw3native\\.h$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/glfw" TYPE FILE FILES
    "/mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/build/external/glfw-3.1.2/src/glfw3Config.cmake"
    "/mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/build/external/glfw-3.1.2/src/glfw3ConfigVersion.cmake"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/glfw/glfwTargets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/glfw/glfwTargets.cmake"
         "/mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/build/external/glfw-3.1.2/CMakeFiles/Export/lib/cmake/glfw/glfwTargets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/glfw/glfwTargets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/glfw/glfwTargets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/glfw" TYPE FILE FILES "/mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/build/external/glfw-3.1.2/CMakeFiles/Export/lib/cmake/glfw/glfwTargets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/glfw" TYPE FILE FILES "/mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/build/external/glfw-3.1.2/CMakeFiles/Export/lib/cmake/glfw/glfwTargets-noconfig.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/build/external/glfw-3.1.2/src/glfw3.pc")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/build/external/glfw-3.1.2/src/cmake_install.cmake")
  include("/mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/build/external/glfw-3.1.2/examples/cmake_install.cmake")
  include("/mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/build/external/glfw-3.1.2/tests/cmake_install.cmake")

endif()

