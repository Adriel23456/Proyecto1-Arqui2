# Install script for directory: D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/SFML-2.6.1/src/SFML

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
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

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/src/SFML/System/cmake_install.cmake")
  include("D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/src/SFML/Main/cmake_install.cmake")
  include("D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/src/SFML/Window/cmake_install.cmake")
  include("D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/src/SFML/Network/cmake_install.cmake")
  include("D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/src/SFML/Graphics/cmake_install.cmake")
  include("D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/src/SFML/Audio/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/src/SFML/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
