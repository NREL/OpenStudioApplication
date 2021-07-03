set(target_name OpenStudioApp)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${QT_INCLUDES})

# source files
set(${target_name}_SRC
  main.cpp
  OpenStudioApp.cpp
  OpenStudioApp.hpp
  StartupView.hpp
  StartupView.cpp
  StartupMenu.hpp
  StartupMenu.cpp
  LibraryDialog.hpp
  LibraryDialog.cpp
  ExternalToolsDialog.hpp
  ExternalToolsDialog.cpp

  ../shared_gui_components/BusyWidget.cpp
  ../shared_gui_components/BusyWidget.hpp
  ../shared_gui_components/OSDialog.cpp
  ../shared_gui_components/OSDialog.hpp
  ../shared_gui_components/WaitDialog.cpp
  ../shared_gui_components/WaitDialog.hpp
)

# moc files
set(${target_name}_moc
  OpenStudioApp.hpp
  StartupView.hpp
  StartupMenu.hpp
  LibraryDialog.hpp
  ExternalToolsDialog.hpp

  ../shared_gui_components/BusyWidget.hpp
  ../shared_gui_components/OSDialog.hpp
  ../shared_gui_components/WaitDialog.hpp
)

## Qt UI generation
qt5_wrap_cpp(${target_name}_moc_src ${${target_name}_moc})

# configure the AboutBox.hpp file
# this needs to be configured to embed the build number
CONFIGURE_FILE_WITH_CHECKSUM(AboutBox.hpp.in "${CMAKE_CURRENT_BINARY_DIR}/AboutBox.hpp")

set(OPENSTUDIOAPPLICATION_ICON_FOLDER "${PROJECT_SOURCE_DIR}/icons")
if(WIN32)
  remove_definitions(/bigobj)
  CONFIGURE_FILE_WITH_CHECKSUM(OpenStudioApp.rc.in "${CMAKE_CURRENT_BINARY_DIR}/OpenStudioApp.rc")
  set(ICON_SRC "${CMAKE_CURRENT_BINARY_DIR}/OpenStudioApp.rc")
elseif(APPLE)
  # For Apple set the icns file containing icons

  # set the name of the bundle icon
  set(MACOSX_BUNDLE_ICON_FILE os.icns)
  # set all icons
  set(ICON_SRC
    ${OPENSTUDIOAPPLICATION_ICON_FOLDER}/os.icns
    ${OPENSTUDIOAPPLICATION_ICON_FOLDER}/osm.icns
    ${OPENSTUDIOAPPLICATION_ICON_FOLDER}/osc.icns
  )

  set_source_files_properties(${ICON_SRC}
    PROPERTIES MACOSX_PACKAGE_LOCATION Resources
  )

  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${OPENSTUDIOAPPLICATION_VERSION}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${OPENSTUDIOAPPLICATION_LONG_VERSION}")
  set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2008-2019, Alliance for Sustainable Energy, LLC, and other contributors. All rights reserved.")

  set(MACOSX_BUNDLE_EXECUTABLE_NAME ${target_name})
  # PROJECT_VERSION_BUILD = Git SHA. This makes it globally unique, and is what we want to avoid CFBUndleIdenfitied collision
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "gov.nrel.openstudio.${PROJECT_VERSION_BUILD}.openstudio")
  set(MACOSX_BUNDLE_BUNDLE_NAME "OpenStudioApplication")
  CONFIGURE_FILE_WITH_CHECKSUM(${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)

elseif(UNIX)
  # Install the icons
  # Cf. http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html#directory_layout

  # NOTE: Instead, to avoid dpkg issuing conflicts when installing several different versions, we use a post-installation script
  # So install them to a temp directory that we'll remove later
  install(DIRECTORY ${OPENSTUDIOAPPLICATION_ICON_FOLDER} DESTINATION Temp
          FILES_MATCHING PATTERN "*.png")

  #foreach(_size 16 32 48 64 128 256)
    ## Copy OS App icons:
    #install(
      #FILES "${OPENSTUDIOAPPLICATION_ICON_FOLDER}/os_${_size}.png"
      #DESTINATION "/usr/share/icons/hicolor/${_size}x${_size}/apps"
      #RENAME openstudioapp.png
    #)
    ## Install OSM mimetypes icons
    #install(
      #FILES "${OPENSTUDIOCORE_ICON_FOLDER}/osm_${_size}.png"
      #DESTINATION "/usr/share/icons/hicolor/${_size}x${_size}/mimetypes"
      #RENAME application-x-openstudio.png
    #)
  ## Install OSC
    #install(
      #FILES "${OPENSTUDIOCORE_ICON_FOLDER}/osc_${_size}.png"
      #DESTINATION "/usr/share/icons/hicolor/${_size}x${_size}/mimetypes"
      #RENAME application-x-openstudio-component.png
    #)

  #endforeach()

  ## OS App has a 512 and 1024 too
  #foreach(_size 512 1024)
    ## Copy OS App icons
    #install(
      #FILES "${OPENSTUDIOAPPLICATION_ICON_FOLDER}/os_${_size}.png"
      #DESTINATION "/usr/share/icons/hicolor/${_size}x${_size}/apps"
      #RENAME openstudioapp.png
    #)
  #endforeach()

endif()

#include_directories(SYSTEM ${RUBY_INCLUDE_DIRS})

add_executable(${target_name}
  WIN32
  MACOSX_BUNDLE
  ${${target_name}_SRC}
  ${${target_name}_moc_src}
  ${${target_name}_moc}
  ${ICON_SRC}
)

if (NINJA)
  target_compile_definitions(${target_name} PRIVATE NINJA=1)
elseif(WIN32)
  # increase stack size
  target_link_options(${target_name} PRIVATE /STACK:8000000)
endif()

# This is because Apple has app bundles for .apps.
# We copy the openstudio cli into the app bundle in an effort
# to be self contained. It also makes it easy to find the cli since
# it will be sitting next to the app executable.
if( APPLE )
add_custom_command(TARGET ${target_name}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:openstudio::openstudio> $<TARGET_FILE_DIR:${target_name}>/$<TARGET_FILE_NAME:openstudio::openstudio>
)
endif()

add_custom_command(TARGET ${target_name}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/platforms/
  #COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/sceneparsers/
  #COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::GLTFSceneParserPlugin> $<TARGET_FILE_DIR:${target_name}>/sceneparsers/
  #COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::AssimpParserPlugin> $<TARGET_FILE_DIR:${target_name}>/sceneparsers/
)

if(WIN32)
  add_custom_command(TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/styles/
  )
endif()

if(UNIX AND NOT APPLE)
  add_custom_command(TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/xcbglintegrations/
  )
endif()

if( APPLE )
  add_custom_command(TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::QCocoaIntegrationPlugin> $<TARGET_FILE_DIR:${target_name}>/platforms/ )
elseif( UNIX )
  foreach(LIB ${QT_PLUGINS})
    get_property(LIB_LOCATION TARGET ${LIB} PROPERTY LOCATION)
    get_filename_component(LIB_BASE_NAME ${LIB_LOCATION} NAME)
    add_custom_command(TARGET ${target_name}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIB_LOCATION} $<TARGET_FILE_DIR:${target_name}>/platforms
      COMMAND patchelf --set-rpath "$ORIGIN/.." $<TARGET_FILE_DIR:${target_name}>/platforms/${LIB_BASE_NAME}
      VERBATIM
    )
    list(APPEND QT_PLUGIN_LOCATIONS $<TARGET_FILE_DIR:${target_name}>/platforms/${LIB_BASE_NAME})
  endforeach()

  foreach(LIB ${QT_LIBS} ${QT_WEB_LIBS})
    get_property(LIB_LOCATION TARGET ${LIB} PROPERTY LOCATION)
    list(APPEND QT_LIB_LOCATIONS "${LIB_LOCATION}")
  endforeach()

  foreach(LIB_LOCATION ${QT_LIB_LOCATIONS} ${QT_EXTRA_LIBS})
    # In case we grabed "xx.so", it'll be resolved to "xx.so.X.Y.Z" as we expect
    get_filename_component(LIB_LOCATION_RESOLVED "${LIB_LOCATION}" REALPATH)

    get_filename_component(LIB_BASE_NAME ${LIB_LOCATION_RESOLVED} NAME)
    string(REGEX REPLACE "\\.[^.]*$" "" LIB_MAJOR_MINOR_NAME ${LIB_BASE_NAME})
    string(REGEX REPLACE "\\.[^.]*$" "" LIB_MAJOR_NAME ${LIB_MAJOR_MINOR_NAME})

    list(APPEND QT_LIB_SYMLINKS "${LIB_MAJOR_MINOR_NAME}" "${LIB_MAJOR_NAME}")

    add_custom_command(TARGET ${target_name}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIB_LOCATION_RESOLVED} $<TARGET_FILE_DIR:${target_name}>/
      COMMAND ln -sf "${LIB_BASE_NAME}" "$<TARGET_FILE_DIR:${target_name}>/${LIB_MAJOR_MINOR_NAME}"
      COMMAND ln -sf "${LIB_BASE_NAME}" "$<TARGET_FILE_DIR:${target_name}>/${LIB_MAJOR_NAME}"
      COMMAND patchelf --set-rpath "$ORIGIN/." $<TARGET_FILE_DIR:${target_name}>/${LIB_BASE_NAME}
      VERBATIM
    )
  endforeach()

  add_custom_command(TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QtWebEngineProcess} $<TARGET_FILE_DIR:${target_name}>/
    COMMAND chrpath -r "$ORIGIN/." $<TARGET_FILE_DIR:${target_name}>/QtWebEngineProcess
    VERBATIM
  )

  add_custom_command(TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/qt.conf $<TARGET_FILE_DIR:${target_name}>
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/resources/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${icudtl} $<TARGET_FILE_DIR:${target_name}>/resources/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${qweb_resources} $<TARGET_FILE_DIR:${target_name}>/resources/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${qweb_resources_devtools} $<TARGET_FILE_DIR:${target_name}>/resources/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${qweb_resources_100} $<TARGET_FILE_DIR:${target_name}>/resources/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${qweb_resources_200} $<TARGET_FILE_DIR:${target_name}>/resources/
    #COMMAND ${CMAKE_COMMAND} -E copy_directory ${QT_INSTALL_DIR}/translations $<TARGET_FILE_DIR:${target_name}>/translations/
    # Copying just one locale is enough to shut the annoying console warning...
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/translations/
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/translations/qtwebengine_locales
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_INSTALL_DIR}/translations/qtwebengine_locales/en-US.pak $<TARGET_FILE_DIR:${target_name}>/translations/qtwebengine_locales/
  )

  # TODO: not sure if chrpath is needed
  add_custom_command(TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::QXcbGlxIntegrationPlugin> $<TARGET_FILE_DIR:${target_name}>/xcbglintegrations/
    COMMAND chrpath -r "$ORIGIN/." $<TARGET_FILE_DIR:${target_name}>/xcbglintegrations/$<TARGET_FILE_NAME:Qt5::QXcbGlxIntegrationPlugin>
    VERBATIM
  )

elseif( WIN32 )
  add_custom_command(TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::QWindowsIntegrationPlugin> $<TARGET_FILE_DIR:${target_name}>/platforms/ )

  add_custom_command(TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::QWindowsVistaStylePlugin> $<TARGET_FILE_DIR:${target_name}>/styles/ )

  foreach(LIB ${QT_LIBS} ${QT_WEB_LIBS})
    # release dlls are needed in debug dir for QtWebEngineProcess, second copy_if_different will be a no-op
    get_property(LIB_RELEASE_PATH TARGET ${LIB} PROPERTY IMPORTED_LOCATION_RELEASE)
    add_custom_command(TARGET ${target_name}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${LIB}> $<TARGET_FILE_DIR:${target_name}>/
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIB_RELEASE_PATH} $<TARGET_FILE_DIR:${target_name}>/
    )
  endforeach()

  target_link_libraries(${target_name} optimized ${QT_MAIN_LIB} debug ${QT_MAIN_LIB_D})

  add_custom_command(TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QtWebEngineProcess} $<TARGET_FILE_DIR:${target_name}>/ )

  add_custom_command(TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/qt.conf $<TARGET_FILE_DIR:${target_name}>
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/resources/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${icudtl} $<TARGET_FILE_DIR:${target_name}>/resources/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${qweb_resources} $<TARGET_FILE_DIR:${target_name}>/resources/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${qweb_resources_devtools} $<TARGET_FILE_DIR:${target_name}>/resources/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${qweb_resources_100} $<TARGET_FILE_DIR:${target_name}>/resources/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${qweb_resources_200} $<TARGET_FILE_DIR:${target_name}>/resources/
    #COMMAND ${CMAKE_COMMAND} -E copy_directory ${QT_INSTALL_DIR}/translations $<TARGET_FILE_DIR:${target_name}>/translations/
    # Copying just one locale is enough to shut the annoying console warning...
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/translations/
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/translations/qtwebengine_locales
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_INSTALL_DIR}/translations/qtwebengine_locales/en-US.pak $<TARGET_FILE_DIR:${target_name}>/translations/qtwebengine_locales/
  )

  add_custom_command(TARGET ${target_name}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:openstudio::openstudiolib> $<TARGET_FILE_DIR:${target_name}>/ )

endif()

set(depends
  openstudio_lib
  openstudio_bimserver
  openstudio_modeleditor
  openstudioapp_utilities
  openstudio::openstudiolib
  ${QT_WEB_LIBS}
)

target_link_libraries(${target_name} ${depends})

#AddPCH(${target_name})

#------------------------------------------------------------------------------
# Installation
#------------------------------------------------------------------------------
if(APPLE)
  set_target_properties(${target_name} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)
  set(plugin_dest_dir OpenStudioApp.app/Contents/MacOS)
  set(APPS "\${CMAKE_INSTALL_PREFIX}/OpenStudioApp.app")
elseif(WIN32)
  set(plugin_dest_dir bin)
  set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/OpenStudioApp.exe")
else()
  set_target_properties(${target_name} PROPERTIES INSTALL_RPATH "$ORIGIN/.")
  set(plugin_dest_dir bin)
  set(APPS "\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin/OpenStudioApp")
endif()

if( APPLE )
  install(TARGETS ${target_name}
    DESTINATION .
    COMPONENT "OpenStudioApp"
  )
else()
  install(TARGETS ${target_name}
    DESTINATION bin
    COMPONENT "OpenStudioApp"
  )
endif()

# directories to look for dependencies
get_target_property(os_lib_location openstudio::openstudiolib LOCATION)
get_filename_component(os_lib_dir ${os_lib_location} DIRECTORY)
set(DIRS "${QT_INSTALL_DIR}/bin" "${QT_INSTALL_DIR}/lib" "${os_lib_dir}")

if(APPLE)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/openstudio_app/Resources/
          DESTINATION OpenStudioApp.app/Contents/Resources
          COMPONENT "Resources"
  )
elseif(WIN32)
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/openstudio_app/Resources/
    DESTINATION Resources
    COMPONENT "Resources"
  )
else()
  install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/openstudio_app/Resources/
    DESTINATION Resources
    COMPONENT "Resources"
  )
endif()

# since on apple the whole app bundle is installed (including these webkit components) we can exclude apple
if( NOT APPLE )

  install(FILES $<TARGET_FILE_DIR:${target_name}>/qt.conf DESTINATION bin COMPONENT OpenStudioApp)
  # install(DIRECTORY ${QT_INSTALL_DIR}/translations DESTINATION bin COMPONENT OpenStudioApp)
  install(DIRECTORY $<TARGET_FILE_DIR:${target_name}>/translations/ DESTINATION bin/translations/ COMPONENT OpenStudioApp)
  install(DIRECTORY $<TARGET_FILE_DIR:${target_name}>/resources/ DESTINATION bin/resources/ COMPONENT OpenStudioApp)
  # install(FILES ${icudtl} DESTINATION bin/resources COMPONENT OpenStudioApp )/translatio
  # install(FILES ${qweb_resources} DESTINATION bin/resources COMPONENT OpenStudioApp )
  # install(FILES ${qweb_resources_100} DESTINATION bin/resources COMPONENT OpenStudioApp )
  # install(FILES ${qweb_resources_200} DESTINATION bin/resources COMPONENT OpenStudioApp )
  if(WIN32)
    install(TARGETS OpenStudioApp DESTINATION bin COMPONENT OpenStudioApp )

    install(PROGRAMS ${QtWebEngineProcess} DESTINATION bin COMPONENT OpenStudioApp )
    install(PROGRAMS $<TARGET_FILE:Qt5::QWindowsIntegrationPlugin> DESTINATION bin/platforms/ COMPONENT OpenStudioApp)
    install(PROGRAMS $<TARGET_FILE:Qt5::QWindowsVistaStylePlugin> DESTINATION bin/styles/ COMPONENT OpenStudioApp)
    #install(PROGRAMS $<TARGET_FILE:Qt5::GLTFSceneParserPlugin> DESTINATION bin/sceneparsers/ COMPONENT OpenStudioApp)
    #install(PROGRAMS $<TARGET_FILE:Qt5::AssimpParserPlugin> DESTINATION bin/sceneparsers/ COMPONENT OpenStudioApp)
  elseif(UNIX)
    install(PROGRAMS $<TARGET_FILE_DIR:${target_name}>/QtWebEngineProcess DESTINATION bin/ COMPONENT OpenStudioApp)
    #install(PROGRAMS $<TARGET_FILE_DIR:${target_name}>/platforms/$<TARGET_FILE_NAME:Qt5::QXcbIntegrationPlugin> DESTINATION bin/platforms/ COMPONENT OpenStudioApp)
    install(PROGRAMS $<TARGET_FILE_DIR:${target_name}>/xcbglintegrations/$<TARGET_FILE_NAME:Qt5::QXcbGlxIntegrationPlugin> DESTINATION bin/xcbglintegrations/ COMPONENT OpenStudioApp)
    #install(PROGRAMS $<TARGET_FILE_DIR:${target_name}>/sceneparsers/$<TARGET_FILE_NAME:Qt5::GLTFSceneParserPlugin> DESTINATION bin/sceneparsers/ COMPONENT OpenStudioApp)
    #install(PROGRAMS $<TARGET_FILE_DIR:${target_name}>/sceneparsers/$<TARGET_FILE_NAME:Qt5::AssimpParserPlugin> DESTINATION bin/sceneparsers/ COMPONENT OpenStudioApp)
    #install(PROGRAMS $<TARGET_FILE_DIR:${target_name}>/libQt5XcbQpa.so.5 DESTINATION bin/ COMPONENT OpenStudioApp)

    foreach(LIB ${QT_LIBS} ${QT_WEB_LIBS})
      install(PROGRAMS $<TARGET_FILE_DIR:${target_name}>/$<TARGET_FILE_NAME:${LIB}> DESTINATION bin/ COMPONENT OpenStudioApp)
    endforeach()

    foreach(LIB_LOCATION ${QT_EXTRA_LIBS})
      get_filename_component(LIB_LOCATION_RESOLVED "${LIB_LOCATION}" REALPATH)
      get_filename_component(LIB_BASE_NAME ${LIB_LOCATION_RESOLVED} NAME)
      install(PROGRAMS $<TARGET_FILE_DIR:${target_name}>/${LIB_BASE_NAME} DESTINATION bin/ COMPONENT OpenStudioApp)
    endforeach()


    foreach(LINK ${QT_LIB_SYMLINKS})
      install(FILES $<TARGET_FILE_DIR:${target_name}>/${LINK} DESTINATION bin/ COMPONENT OpenStudioApp)
    endforeach()

    foreach(PLUGIN ${QT_PLUGIN_LOCATIONS})
      install(FILES ${PLUGIN} DESTINATION bin/platforms/ COMPONENT OpenStudioApp)
    endforeach()

  endif()
endif()

if( APPLE )
  list(APPEND QTPLUGINS "\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/platforms/libqcocoa.dylib")
  #list(APPEND QTPLUGINS "\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/sceneparsers/libgltfsceneparser.dylib")
  #list(APPEND QTPLUGINS "\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/sceneparsers/libassimpsceneparser.dylib")
  install(DIRECTORY ${QtWebEngineProcess} DESTINATION "OpenStudioApp.app/Contents/Frameworks/QtWebEngineCore.framework/Versions/5/Helpers/" COMPONENT OpenStudioApp USE_SOURCE_PERMISSIONS)
  list(APPEND QTPLUGINS "\${CMAKE_INSTALL_PREFIX}/OpenStudioApp.app/Contents/Frameworks/QtWebEngineCore.framework/Versions/5/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess")
endif()

# This does some "magic" **including on Linux** (cf #345) (On linux it will install in ./bin/ the libopenstudio.so, and libicu* stuff)
install(CODE "
  # APPS might be a symbolic link which does not work with fixup_bundle
  # expand APPS to a real path
  get_filename_component(APPS_EXPANDED ${APPS} REALPATH)
  include(BundleUtilities)
  # This file defines some libs we are excluding. Some like concrt140.dll are installed already via InstallRequiredSystemLibraries
  include(\"${CMAKE_SOURCE_DIR}/CMake/FixupBundleOverrides.cmake\")
  fixup_bundle(\"\${APPS_EXPANDED}\" \"${QTPLUGINS}\" \"${DIRS}\")
  " COMPONENT "OpenStudioApp"
)

set(${target_name}_test_src
  test/OpenStudioAppFixture.hpp
  test/OpenStudioAppFixture.cpp
  test/Resources_GTest.cpp
  test/Units_GTest.cpp
)

CREATE_SRC_GROUPS("${${target_name}_test_src}")
if(BUILD_TESTING)
  set(${target_name}_test_depends
    ${depends}
  )
  CREATE_TEST_TARGETS(${target_name} "${${target_name}_test_src}" "${${target_name}_test_depends}")

  #add_dependencies("${target_name}_tests"
  #  openstudio_app_resources
  #)

endif()
