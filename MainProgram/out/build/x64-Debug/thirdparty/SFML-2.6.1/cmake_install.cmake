# Install script for directory: D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/SFML-2.6.1

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

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE DIRECTORY FILES "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/SFML-2.6.1/include" FILES_MATCHING REGEX "/[^/]*\\.hpp$" REGEX "/[^/]*\\.inl$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE DIRECTORY FILES "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/lib" FILES_MATCHING REGEX "/[^/]*\\.pdb$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/SFML" TYPE FILE FILES "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/SFML-2.6.1/license.md")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/SFML" TYPE FILE FILES "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/SFML-2.6.1/readme.md")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE DIRECTORY FILES "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/SFML-2.6.1/extlibs/bin/x64/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE DIRECTORY FILES "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/SFML-2.6.1/extlibs/libs-msvc-universal/x64/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML/SFMLStaticTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML/SFMLStaticTargets.cmake"
         "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/CMakeFiles/Export/3937c6824958577f216dad0a66bc6149/SFMLStaticTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML/SFMLStaticTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML/SFMLStaticTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML" TYPE FILE FILES "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/CMakeFiles/Export/3937c6824958577f216dad0a66bc6149/SFMLStaticTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML" TYPE FILE FILES "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/CMakeFiles/Export/3937c6824958577f216dad0a66bc6149/SFMLStaticTargets-debug.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML" TYPE FILE FILES
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/SFMLConfig.cmake"
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/SFMLConfigDependencies.cmake"
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/SFMLConfigVersion.cmake"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/src/SFML/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/SFML-2.6.1/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
