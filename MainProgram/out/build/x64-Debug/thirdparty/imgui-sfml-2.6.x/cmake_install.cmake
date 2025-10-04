# Install script for directory: D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/imgui-sfml-2.6.x

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

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/imgui-sfml-2.6.x/ImGui-SFML.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/imgui-sfml-2.6.x/imgui-SFML.h"
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/imgui-sfml-2.6.x/imgui-SFML_export.h"
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/imgui-docking/imgui/imconfig.h"
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/imgui-docking/imgui/imgui.h"
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/imgui-docking/imgui/imgui_internal.h"
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/imgui-docking/imgui/imstb_rectpack.h"
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/imgui-docking/imgui/imstb_textedit.h"
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/imgui-docking/imgui/imstb_truetype.h"
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/imgui-docking/imgui/misc/cpp/imgui_stdlib.h"
    "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/thirdparty/imgui-sfml-2.6.x/imconfig-SFML.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ImGui-SFML/ImGui-SFMLConfig.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ImGui-SFML/ImGui-SFMLConfig.cmake"
         "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/imgui-sfml-2.6.x/CMakeFiles/Export/761a6a4c7704629aea6d1d08969b2ac8/ImGui-SFMLConfig.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ImGui-SFML/ImGui-SFMLConfig-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/ImGui-SFML/ImGui-SFMLConfig.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ImGui-SFML" TYPE FILE FILES "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/imgui-sfml-2.6.x/CMakeFiles/Export/761a6a4c7704629aea6d1d08969b2ac8/ImGui-SFMLConfig.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/ImGui-SFML" TYPE FILE FILES "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/imgui-sfml-2.6.x/CMakeFiles/Export/761a6a4c7704629aea6d1d08969b2ac8/ImGui-SFMLConfig-debug.cmake")
  endif()
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "D:/Escritorio/Informacion-de-Prioridad/TEC/2025/Semestre2/Arqui2/Evaluaciones/Proyecto1/Proyecto1-Arqui2/MainProgram/out/build/x64-Debug/thirdparty/imgui-sfml-2.6.x/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
