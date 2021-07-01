cmake_minimum_required(VERSION 3.10.2)
cmake_policy(SET CMP0048 NEW)

set(CMAKE_CXX_STANDARD 17)

# Use ccache is available, has to be before "project()"
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  # Support Unix Makefiles and Ninja
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

project(OpenStudioApplication VERSION 1.2.1)

# Check system info globally so we can use it everywhere after: Has to be done before FindOpenStudioSDK.cmake
if(APPLE)
  # Looking at cmake source code OS_RELEASE is already set to the output of `sw_vers -productVersion` which is what we want
  cmake_host_system_information(RESULT OSX_VERSION QUERY OS_RELEASE)
  message("-- OS_RELEASE variable is set to: " ${OSX_VERSION})
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET STREQUAL "")
    message("Using CMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    set(OSX_VERSION "${CMAKE_OSX_DEPLOYMENT_TARGET}")
  endif()

  # The output is like 10.12.6 or 10.13, let's strip the end component if any
  string(REGEX REPLACE "^([0-9]+\\.[0-9]+)\\.?.*" "\\1" OSX_VERSION_MAJOR_MINOR ${OSX_VERSION})

elseif(UNIX)
  # OS_RELEASE is the result of `uname -r` which is unhelpful (eg '5.4.0-42-generic')
  find_program(LSB_RELEASE lsb_release)
  # -rs outputs only 16.04, or 18.04
  execute_process(COMMAND ${LSB_RELEASE} -rs OUTPUT_VARIABLE LSB_RELEASE_VERSION_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)

  # -is outputs "Ubuntu" or "Fedora"
  execute_process(COMMAND ${LSB_RELEASE} -is OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)

endif()

# set openstudio_DIR to the directory that contains the openstudioConfig.cmake
# eg C:\src\OpenStudio\build-vs\_CPack_Packages\win64\ZIP\OpenStudio-3.0.0-rc2+ce27ae6b85-Windows\lib\cmake\openstudio
# Or it'll download a zip/tar.gz for you
include(FindOpenStudioSDK.cmake)

###############################################################################

###############################################################################
# Conan

# Note JM 2019-04-24: Another option is to globally set "print_run_commands = True" in ~/.conan/conan.conf
option(CONAN_PRINT_RUN_COMMANDS "Log every conan self.run command" OFF)
mark_as_advanced(CONAN_PRINT_RUN_COMMANDS)
if(CONAN_PRINT_RUN_COMMANDS)
  set(ENV{CONAN_PRINT_RUN_COMMANDS} "1")
  message("Setting CONAN_PRINT_RUN_COMMANDS: $ENV{CONAN_PRINT_RUN_COMMANDS}")
else()
  unset(ENV{CONAN_PRINT_RUN_COMMANDS})
  message("CONAN_PRINT_RUN_COMMANDS: $ENV{CONAN_PRINT_RUN_COMMANDS}")
endif()

# Attempt to run, it'll do the right thing if the calling cmake script already ran it
set(CONAN_INSTALL_FILE "${PROJECT_SOURCE_DIR}/ConanInstall.cmake")
include("${CONAN_INSTALL_FILE}")

# A macro to find a conan related value especially when using multi-config builds (MSVC)
# But it also works with single-config builds
macro(FindValue ValueName)
  set(LocalVar "")
  set(LocalVar $<$<CONFIG:Debug>:${${ValueName}_DEBUG}>$<$<CONFIG:Release>:${${ValueName}_RELEASE}>$<$<CONFIG:RelWithDebInfo>:${$ValueName}_RELWITHDEBINFO}>$<$<CONFIG:MinSizeRel>:${${ValueName}_MINSIZEREL}>
  )
#  list(JOIN LocalVar "" LocalVar)
  string(STRIP ${LocalVar} LocalVar)
  set(CURRENT_${ValueName} $<IF:$<BOOL:${LocalVar}>,${LocalVar},${${ValueName}}>)
  # For debug purposes
  # message(STATUS "Found '${ValueName}' as '${CURRENT_${ValueName}}'")
endmacro()

###############################################################################

# The PROJECT_SOURCE_DIR here is OpenStudioApplication/, which we will use instead of using CMAKE_SOURCE_DIR
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
# We will use BINARY_DIR instead of CMAKE_BINARY_DIR
set(PROJECT_BINARY_DIR "${CMAKE_BINARY_DIR}")

#MESSAGE(FATAL_ERROR "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}, BINARY_DIR=${BINARY_DIR}, PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}, CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}, CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}")

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

if ( APPLE )
  find_library(SECURITY_FRAMEWORK Security)
endif()

include(ExternalProject)
include(CPackComponent)
include(CheckCXXCompilerFlag)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0022)
  cmake_policy(SET CMP0022 NEW)
endif()
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()
if(POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif()

if(BUILD_MICRO_PROFILING)
  if(MSVC)
    # Install micro-profiler extension https://github.com/tyoma/micro-profiler
    # Locate micro-profiler_x64.lib under %LocalAppData%\Microsoft\VisualStudio\<Visual Studio version>\Extensions\
    # Copy dbghelp_x64.dll from extension into directory containing executable to profile
    find_library(MICRO_PROFILER_LIB micro-profiler_x64)
  endif()
endif()

###############################################################################
#                                  N I N J A                                  #
###############################################################################
# Ninja support: has to be atop for it to take effect before anything else is done
# Add Color Output if Using Ninja
macro(AddCXXFlagIfSupported flag test)
  CHECK_CXX_COMPILER_FLAG(${flag} ${test})
  if(${${test}})
    message("adding ${flag}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
  endif()
endmacro()

if("Ninja" STREQUAL ${CMAKE_GENERATOR})
  # Will be passed to OpenStudio.in to deal with the Win32 case
  set(NINJA TRUE)

  # Clang
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    AddCXXFlagIfSupported(-fcolor-diagnostics COMPILER_SUPPORTS_fcolor-diagnostics)
  endif()

  # g++
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # For some reason it doesn't say its supported, but it works...
    # AddCXXFlagIfSupported(-fdiagnostics-color COMPILER_SUPPORTS_fdiagnostics-color)
    message(STATUS "Ninja: Forcing -fdiagnostics-color=always")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
  endif()
endif()

# Xcode/Ninja generators undefined MAKE
if(CMAKE_GENERATOR MATCHES "Make")
  set(MAKE "$(MAKE)")
else()
  set(MAKE make)
endif()


###############################################################################
#                       P R O J E C T    V E R S I O N                        #
###############################################################################
# Project version information

set(PROJECT_VERSION_BUILD "Unknown" CACHE STRING "Build number") # git sha
find_package(Git)

if(NOT GIT_FOUND)
  find_program(GIT_EXECUTABLE git HINTS "$ENV{LOCALAPPDATA}/Programs/git/bin" "C:/Program Files/Git/bin")
  if(NOT GIT_EXECUTABLE_NOTFOUND)
    set(GIT_FOUND TRUE)
  endif()
endif()

if(GIT_FOUND)
  # Git SHA (length 10)
  execute_process(COMMAND "${GIT_EXECUTABLE}" "rev-parse" "--short=10" "HEAD"
                  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                  TIMEOUT 10
                  RESULT_VARIABLE RESULT
                  OUTPUT_VARIABLE GIT_VERSION
                  ERROR_QUIET
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(${RESULT} EQUAL 0 AND NOT "${GIT_VERSION}" EQUAL "${PROJECT_VERSION_BUILD}")
    set(PROJECT_VERSION_BUILD ${GIT_VERSION} CACHE STRING "Build number" FORCE) # git sha
  endif()

  # Git branch name
  execute_process(COMMAND "${GIT_EXECUTABLE}" "rev-parse" "--abbrev-ref" "HEAD"
                  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                  TIMEOUT 10
                  RESULT_VARIABLE RESULT
                  OUTPUT_VARIABLE GIT_BRANCH
                  ERROR_QUIET
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(${RESULT} EQUAL 0 AND NOT "${GIT_BRANCH}" EQUAL "${PROJECT_GIT_BRANCH}")
    set(PROJECT_GIT_BRANCH ${GIT_BRANCH} CACHE STRING "Git Branch Name" FORCE)
  endif()

  get_filename_component(GIT_DIR "${GIT_EXECUTABLE}" DIRECTORY)
else()
  set(GIT_DIR "")
endif()

#find_program(PATCH_EXE patch HINTS "${GIT_DIR}" "${GIT_DIR}/../bin/" "${GIT_DIR}/../usr/bin/")
#string(COMPARE EQUAL "${PATCH_EXE}" "PATCH_EXE-NOTFOUND" PATCH_EXE_NOTFOUND)
#if(PATCH_EXE_NOTFOUND)
  #message(SEND_ERROR "Required program patch not found")
#endif()

# README: what's the difference between all these versions?

# The "OpenStudioApplicaton_VERSION" ones should be used in CMakeLists.txt
# * OpenStudioApplicaton_VERSION: includes the prerelease tag if any: '1.1.0-rc1'
# * OpenStudioApplicaton_VERSION_PATCH: Patch + prelrease tag if any: '0-rc1'

# The "OPENSTUDIOAPPLICATION_VERSION" ones should only be used in manifests files (aside from using the the long version for naming the installer)
# * OPENSTUDIOAPPLICATION_VERSION: just major.minor.patch: '1.1.0'
# * OPENSTUDIO_LONG_VERSION: includes prelrease and build sha: "1.1.0-rc1+ejfbxhza'

# TODO: Modify the more specific variables as needed to indicate prerelease, etc
# Keep in beta in-between release cycles. Set to empty string (or comment out) for official)
set(PROJECT_VERSION_PRERELEASE "")

# OpenStudio version: Only include Major.Minor.Patch, eg "3.0.0", even if you have a prerelease tag
set(OPENSTUDIOAPPLICATION_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

if (NOT "${PROJECT_VERSION_PRERELEASE}" STREQUAL "")
  message(STATUS "We have a prerelease tag set to '${PROJECT_VERSION_PRERELEASE}'")
  if("${PROJECT_GIT_BRANCH}" STREQUAL "master")
    message(AUTHOR_WARNING "You are building branch 'master', perhaps you shouldn't have included a pre-release tag")
  endif()
  string(APPEND OpenStudioApplication_VERSION "-${PROJECT_VERSION_PRERELEASE}")
  string(APPEND OpenStudioApplication_VERSION_PATCH "-${PROJECT_VERSION_PRERELEASE}")
  # Long version, include prerelease and build metadata. eg '1.1.0-rc1+6383a191dc'
  set(OPENSTUDIOAPPLICATION_LONG_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}-${PROJECT_VERSION_PRERELEASE}+${PROJECT_VERSION_BUILD}")
else()
  message(STATUS "We do not have a prerelease tag")
  if(NOT "${PROJECT_GIT_BRANCH}" STREQUAL "master")
    message(AUTHOR_WARNING "You are building another branch than master but you do not have a prerelease tag, consider adding one such as 'beta'")
  endif()
  # Long version, include build metadata. eg '3.0.0+6383a191dc' for an official release
  set(OPENSTUDIOAPPLICATION_LONG_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}+${PROJECT_VERSION_BUILD}")
endif()

###############################################################################
#                         B U I L D    O P T I O N S                          #
###############################################################################

if(UNIX AND NOT APPLE)
  # the RPATH to be used when installing
  set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

option(BUILD_PACKAGE "Build package" OFF)

option(BUILD_TESTING "Build testing" OFF)

option(BUILD_BENCHMARK "Build benchmarking targets" OFF)

option(BUILD_MICRO_PROFILING "Build micro profiling" OFF)

# DView
# option(BUILD_DVIEW "Build DView" OFF)

###############################################################################
#                         C M A K E    C O N T R O L                          #
###############################################################################

# High level project configuration
# Do we actually want everything to go to CMAKE_BINARY_DIR/Products,
# so that when you build OpenStudioApplication you get both OpenStudio (core) and Application in the same place?
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Products")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Products")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Products")

set(LIBRARY_SEARCH_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug")

# TODO: move App macros to a local ProjectMacros.cmake
# Include project specific macros
include("${PROJECT_SOURCE_DIR}/ProjectMacros.cmake")


# Search first in the binary dir, where conan will install finders, then
# search for modules in the root dir to override cmake ones
list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR} ${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/CMake")

# Use CTest
set(ALL_TESTING_TARGETS "") # global list
if(BUILD_TESTING)
  enable_testing()
  include(CTest)
endif()


###############################################################################
#          C O M P I L E R    A N D    S Y S T E M    O P T I O N S           #
###############################################################################
# Compiler and system specific options

if(MSVC AND NOT NINJA)
  # Build with Multiple Processes
  set(BUILD_WITH_MULTIPLE_PROCESSES ON CACHE BOOL "/MP compiler flag for full processor utilization")
  mark_as_advanced(BUILD_WITH_MULTIPLE_PROCESSES)
endif()


if(UNIX)

  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC -fno-strict-aliasing")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fno-strict-aliasing")

  # all warnings
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -fno-strict-aliasing -Winvalid-pch -Wnon-virtual-dtor")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC -fno-strict-aliasing -Winvalid-pch")
  # Treat all warnings as errors
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overloaded-virtual -ftemplate-depth=1024")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overloaded-virtual")
    # DLM: had to add this due to issues with boost optional and gcc, may be resolved if we move to std::optional
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized")
  endif()

  # Note: CMAKE_CXX_STANDARD set to 17 should already take care of adding -std=c++17 or equivalent
  if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -stdlib=libc++")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
  endif()
endif()
if(MINGW)
  # all warnings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  # effective c++
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weffc++")
  # treat warnings as errors
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")

  # wchar not supported for MinGW
  #add_definitions(-DBOOST_LOG_USE_CHAR)
  #add_definitions(-DBOOST_LOG_NO_COMPILER_TLS)
endif()
if(MSVC)
  # warning level 3
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")

  # warning level 4 - DLM: we should shoot for this
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

  # all warnings - DLM: probably too high to ever use
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")

  # create compiler processes on all effective processors
  # /MP is useless for ninja and will cause problems. BUILD_WITH_MULTIPLE_PROCESSES shouldn't have been set if ninja but let's be safe
  if(BUILD_WITH_MULTIPLE_PROCESSES AND NOT NINJA)
    add_definitions(/MP)
  endif()

  # Enable warning on thread un-safe static member initialization - DLM: we have been bit by this many times
  # TODO: JM 2019-09-19: Disabling for now
  # Using the righg flag (/w14460 and not the previously mispelled /w44640) will make compilation fail with many warnings treated as error,
  # eg: IddRegex.cpp all scoped `const static std::string result` inside functions will issue that and make it fatal
  # add_definitions(/w14640)

  # ignore decorated name length exceeded
  add_definitions(/wd4503)

  # ignore needs to have dll-interface to be used by clients of class
  # we will not rely on exporting templates instead all libraries must
  # be linked against the same runtime
  add_definitions(/wd4251)

  # ignore base class not dll-exported error
  add_definitions(/wd4275)


  add_definitions(/bigobj)

  if(CMAKE_CL_64)
    # ignore conversion from size_t to int for now
    add_definitions(/wd4267)
  endif()

  # treat warnings as errors
  # DLM: only do this for our code, added in later
  # add_definitions(/WX)

  # ignore warnings about the stl being insecure
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)

  # ignore c++17 deprecation warnings generated by boost
  add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
  add_definitions(-D_SILENCE_FPOS_SEEKPOS_DEPRECATION_WARNING)

  # for profiling with micro-profiler
  if(BUILD_MICRO_PROFILING)
    add_definitions(/GH /Gh /Zi)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG /OPT:REF /OPT:ICF")
  endif()

endif()

# Check version of gcc
if(CMAKE_COMPILER_IS_GNUCC)
  execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                  OUTPUT_VARIABLE GCC_VERSION)
endif()

# compiler id for about boxes
if(MSVC)
  set(ABOUT_COMPILER "${CMAKE_GENERATOR}")
elseif(APPLE)
  set(ABOUT_COMPILER "${CMAKE_CXX_COMPILER_ID} ${CMAKE_OSX_ARCHITECTURES}")
else()
  set(ABOUT_COMPILER "${CMAKE_CXX_COMPILER_ID}")
endif()

###############################################################################
#                           D E P E N D E N C I E S                           #
###############################################################################

# Required dependencies

# Threading library
find_package(Threads)
if(UNIX)
  set(CMAKE_THREAD_LIBS "${CMAKE_THREAD_LIBS_INIT}" CACHE STRING "Thread library used.")
  mark_as_advanced(CMAKE_THREAD_LIBS)
endif()

# Swig
FindValue(CONAN_SWIG_ROOT)
set(SWIG_EXECUTABLE "${CURRENT_CONAN_SWIG_ROOT}/bin/swig")
# The conan-provided binary has a built-in swiglib (`swig -swiglib`) that points to the build box on which it was built, which is problematic for us.
set(SWIG_LIB "${CURRENT_CONAN_SWIG_ROOT}/bin/swiglib")

# Qt
# e.g. QT_INSTALL_DIR = C:/Qt-5.15.0/5.15.0/msvc2019_64
set(QT_INSTALL_DIR "" CACHE PATH "Path to Qt Install")
mark_as_advanced(QT_INSTALL_DIR)
set(QT_VERSION "5.15.0")
#if(NOT EXISTS ${QT_INSTALL_DIR})
  ## download qt
  #if(WIN32)
    #if(CMAKE_CL_64)
      #set(QT_ZIP_FILENAME "qt_5_11_msvc2017_64_shared.zip")
      #set(QT_ZIP_EXPECTED_MD5 "82f44a8decf101ca7cb7efafd899e8ee")
    #else()
      #set(QT_ZIP_FILENAME "DOES_NOT_EXIST_qt_5_11_msvc2017_32_shared.zip")
      #set(QT_ZIP_EXPECTED_MD5 "25abd383e7db2dfd971b4a4ef145002b")
    #endif()
  #elseif(APPLE)
      ## TODO: this is missing a root level "Qt-install" directory.
      #set(QT_ZIP_FILENAME "qt_5_11_osx_shared.tar.gz")
      #set(QT_ZIP_EXPECTED_MD5 "080ff576111d50be5888807055114166")
  #elseif(EXISTS "/etc/redhat-release")
    #set(QT_ZIP_FILENAME "DOES_NOT_EXIST_qt_5_11_redhat_shared.tar.gz")
    #set(QT_ZIP_EXPECTED_MD5 "b0610716854ed91a003347c455b22eb8")
  #else()
    #if(LSB_RELEASE_VERSION_SHORT MATCHES "18.04")
      #set(QT_ZIP_FILENAME "qt_5_11_linux_shared.tar.gz")
      #set(QT_ZIP_EXPECTED_MD5 "33c5a562935612625ec5e41fe4b75e17")
    #else()
      ## 16.04
      #set(QT_ZIP_FILENAME "qt_5_11_linux_shared_xenial.tar.gz")
      #set(QT_ZIP_EXPECTED_MD5 "cb02b8cb34de94038a1a7cba59c0c43a")
    #endif()
  #endif()

  #set(QT_ZIP_LOCAL_PATH "${PROJECT_BINARY_DIR}/${QT_ZIP_FILENAME}")
  #if(EXISTS "${QT_ZIP_LOCAL_PATH}")
    #file(MD5 "${QT_ZIP_LOCAL_PATH}" QT_ZIP_MD5)
  #endif()

  #if(NOT QT_ZIP_MD5 STREQUAL QT_ZIP_EXPECTED_MD5)
    #message(STATUS "Downloading Qt: ${QT_ZIP_FILENAME}")
    #file(DOWNLOAD "http://openstudio-resources.s3.amazonaws.com/dependencies/${QT_ZIP_FILENAME}"
      #${QT_ZIP_LOCAL_PATH}
      #INACTIVITY_TIMEOUT 900
      #SHOW_PROGRESS
      #EXPECTED_MD5 ${QT_ZIP_EXPECTED_MD5})
  ## TODO: TEMP
    #if (APPLE)
      #execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/Qt-install")
      #execute_process(COMMAND ${CMAKE_COMMAND} -E tar xfz ${QT_ZIP_LOCAL_PATH} WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/Qt-install")
    #else()
      #execute_process(COMMAND ${CMAKE_COMMAND} -E tar xfz ${QT_ZIP_LOCAL_PATH} WORKING_DIRECTORY "${PROJECT_BINARY_DIR}")
    #endif()
  #endif()
#endif()

#if(APPLE)
  #set(QT_INSTALL_DIR "${PROJECT_BINARY_DIR}/Qt-install/${QT_VERSION}/clang_64")
#elseif(UNIX)
  #set(QT_INSTALL_DIR "${PROJECT_BINARY_DIR}/Qt-install/${QT_VERSION}/gcc_64")
#elseif(WIN32)
  #set(QT_INSTALL_DIR "${PROJECT_BINARY_DIR}/Qt-install/${QT_VERSION}/win64_msvc2019_64")
#endif()

message(STATUS "QT_INSTALL_DIR=${QT_INSTALL_DIR}")

find_package(Qt5Core ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5Widgets ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5Sql ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5Network ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5Xml ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5Concurrent ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5PrintSupport ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5Gui ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5Quick ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5QuickWidgets ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5Qml ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5WebChannel ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5Positioning ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)

find_package(Qt5WebEngine ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
find_package(Qt5WebEngineWidgets ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
#find_package(Qt53DCore ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
#find_package(Qt53DInput ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
#find_package(Qt53DRender ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)

find_package(Qt5LinguistTools ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)

if(APPLE)
  set(QtWebEngineProcess "${QT_INSTALL_DIR}/lib/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app" CACHE PATH "Path to the QtWebEngineProcess")
else()
  find_program(QtWebEngineProcess NAMES QtWebEngineProcess PATHS "${QT_INSTALL_DIR}/bin/" "${QT_INSTALL_DIR}/libexec/" "${QT_INSTALL_DIR}/lib/QtWebEngineCore.framework/Helpers" NO_DEFAULT_PATH)
endif()
find_file(icudtl NAMES icudtl.dat PATHS "${QT_INSTALL_DIR}/resources/" "${QT_INSTALL_DIR}/lib/QtWebEngineCore.framework/Resources" NO_DEFAULT_PATH)
find_file(qweb_resources NAMES qtwebengine_resources.pak PATHS "${QT_INSTALL_DIR}/resources/" "${QT_INSTALL_DIR}/lib/QtWebEngineCore.framework/Resources" NO_DEFAULT_PATH)
find_file(qweb_resources_devtools NAMES qtwebengine_devtools_resources.pak PATHS "${QT_INSTALL_DIR}/resources/" "${QT_INSTALL_DIR}/lib/QtWebEngineCore.framework/Resources" NO_DEFAULT_PATH)
find_file(qweb_resources_100 NAMES qtwebengine_resources_100p.pak PATHS "${QT_INSTALL_DIR}/resources/" "${QT_INSTALL_DIR}/lib/QtWebEngineCore.framework/Resources" NO_DEFAULT_PATH)
find_file(qweb_resources_200 NAMES qtwebengine_resources_200p.pak PATHS "${QT_INSTALL_DIR}/resources/" "${QT_INSTALL_DIR}/lib/QtWebEngineCore.framework/Resources" NO_DEFAULT_PATH)

# DLM: why do we have QT_WEB_LIBS separate from QT_LIBS?  can we combine these?
# DLM: now the distincton should be between Qt libs linked by openstudio_modeleditor.so and the OS App
list(APPEND QT_WEB_LIBS Qt5::Core)
list(APPEND QT_WEB_LIBS Qt5::Gui)
list(APPEND QT_WEB_LIBS Qt5::WebEngine)
list(APPEND QT_WEB_LIBS Qt5::WebEngineCore)
list(APPEND QT_WEB_LIBS Qt5::WebEngineWidgets)
#list(APPEND QT_WEB_LIBS Qt5::3DCore)
#list(APPEND QT_WEB_LIBS Qt5::3DInput)
#list(APPEND QT_WEB_LIBS Qt5::3DRender)
set_target_properties(${QT_WEB_LIBS} PROPERTIES INTERFACE_LINK_LIBRARIES "")

if(NOT APPLE)

    find_package(Qt5WebChannel ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
    list(APPEND QT_WEB_LIBS Qt5::WebChannel)

    find_package(Qt5Quick ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
    list(APPEND QT_WEB_LIBS Qt5::Quick)

    find_package(Qt5QuickWidgets ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
    list(APPEND QT_WEB_LIBS Qt5::QuickWidgets)

    find_package(Qt5Qml ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
    list(APPEND QT_WEB_LIBS Qt5::Qml)

    find_package(Qt5QmlModels ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
    list(APPEND QT_WEB_LIBS Qt5::QmlModels)

    find_package(Qt5Positioning ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
    list(APPEND QT_WEB_LIBS Qt5::Positioning)

    if(UNIX)
      find_package(Qt5DBus ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
      list(APPEND QT_WEB_LIBS Qt5::DBus)

      # there does not appear to be a normal qt finder for this
      # DLM: actually there is, it is a plugin associated with Qt5::Gui
      #message( Qt5Gui_PLUGINS = ${Qt5Gui_PLUGINS})
      #get_target_property(Qt5QXcbIntegrationPlugin_LOCATION Qt5::QXcbIntegrationPlugin LOCATION)
      #get_target_property(Qt5QXcbIntegrationPlugin_TYPE Qt5::QXcbIntegrationPlugin TYPE)
      #message("Qt5::QXcbIntegrationPlugin Location = ${Qt5QXcbIntegrationPlugin_LOCATION}")
      #message("Qt5::QXcbIntegrationPlugin Type = ${Qt5QXcbIntegrationPlugin_TYPE}")
      list(APPEND QT_PLUGINS Qt5::QXcbIntegrationPlugin)
      # This one doesn't end up in bin/platforms/, but in bin/xcbglintegrations/ so do not append, will be treated separately
      # list(APPEND QT_PLUGINS Qt5::QXcbGlxIntegrationPlugin)
      find_library(QT_QXCBQPA NAMES libQt5XcbQpa.so.${QT_VERSION} PATHS "${QT_INSTALL_DIR}/lib" NO_DEFAULT_PATH)

      find_library(QT_ICU icui18n REQUIRED PATHS "${QT_INSTALL_DIR}/lib" NO_DEFAULT_PATH)
      find_library(QT_ICUDATA icudata REQUIRED PATHS "${QT_INSTALL_DIR}/lib" NO_DEFAULT_PATH)
      find_library(QT_ICUUC icuuc REQUIRED PATHS "${QT_INSTALL_DIR}/lib" NO_DEFAULT_PATH)
      set(QT_ICU_LIBS
        ${QT_ICU}
        ${QT_ICUDATA}
        ${QT_ICUUC}
        )

      list(APPEND QT_EXTRA_LIBS ${QT_QXCBQPA} ${QT_ICU_LIBS})
    endif()
endif()

if(APPLE)
  find_package(Qt5DBus ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
  list(APPEND QT_WEB_LIBS Qt5::DBus)
endif()

list(APPEND QT_LIBS Qt5::Core)
list(APPEND QT_LIBS Qt5::Widgets)
list(APPEND QT_LIBS Qt5::Network)
list(APPEND QT_LIBS Qt5::Xml)
list(APPEND QT_LIBS Qt5::Concurrent)
list(APPEND QT_LIBS Qt5::PrintSupport)
list(APPEND QT_LIBS Qt5::Gui)

if(WIN32)
  find_package(Qt5WinExtras ${QT_VERSION} REQUIRED PATHS ${QT_INSTALL_DIR} NO_DEFAULT_PATH)
  list(APPEND QT_LIBS Qt5::WinExtras)

  find_library(QT_MAIN_LIB NAMES qtmain PATHS "${QT_INSTALL_DIR}/lib" NO_DEFAULT_PATH)
  find_library(QT_MAIN_LIB_D NAMES qtmaind PATHS "${QT_INSTALL_DIR}/lib" NO_DEFAULT_PATH)
endif()

list(APPEND QT_INCLUDES ${Qt5Core_INCLUDE_DIRS})
list(APPEND QT_INCLUDES ${Qt5Concurrent_INCLUDE_DIRS})
list(APPEND QT_INCLUDES ${Qt5Widgets_INCLUDE_DIRS})
list(APPEND QT_INCLUDES ${Qt5Xml_INCLUDE_DIRS})
list(APPEND QT_INCLUDES ${Qt5Network_INCLUDE_DIRS})
list(APPEND QT_INCLUDES ${Qt5Gui_INCLUDE_DIRS})
list(APPEND QT_INCLUDES "${QT_INSTALL_DIR}/include/QtGui/${QT_VERSION}/QtGui") # needed by qtwinmigrate
list(APPEND QT_INCLUDES ${Qt5PrintSupport_INCLUDE_DIRS})

# DLM: added this, but seems to conflict with idea of a separate set of Qt Web dependencies?
list(APPEND QT_INCLUDES ${Qt5Network_INCLUDE_DIRS})
list(APPEND QT_INCLUDES ${Qt5WebEngine_INCLUDE_DIRS})
list(APPEND QT_INCLUDES ${Qt5WebEngineCore_INCLUDE_DIRS})
list(APPEND QT_INCLUDES ${Qt5WebEngineWidgets_INCLUDE_DIRS})

if(UNIX)
  list(APPEND QT_INCLUDES ${Qt5XcbQpa_INCLUDE_DIRS})
endif()

set(CMAKE_AUTOMOC OFF)

###############################################################################
#                         I N C L U D E    P A T H S                          #
###############################################################################
# Add to include path

include_directories("${PROJECT_SOURCE_DIR}/")
include_directories("${PROJECT_SOURCE_DIR}/src/")

# Project binary directory
include_directories("${PROJECT_BINARY_DIR}/")
include_directories("${PROJECT_BINARY_DIR}/src/")

###############################################################################
#                         S U B D I R E C T O R I E S                         #
###############################################################################
# Add project sub directories

if(MSVC)
  # treat warnings as errors
  add_definitions(/WX)
elseif()
  add_definitions(-Werror)
endif()

###############################################################################
#                                T A R G E T S                                #
###############################################################################

# The openstudio::openstudio target (which is the cli) includes a version string
# in the file name. We don't want that degree of coupling between app and sdk
# (we want the app to be able to use different sdk version without recompile,
# so this project (the app) will install and identify the cli generically as
# openstudio or openstudio.exe on windows. The os_cli_install_name is configured
# into the utilities target so we need to set it before the uilities target is defined
get_target_property(os_cli_location openstudio::openstudio LOCATION)
set(os_cli_install_name "openstudio${CMAKE_EXECUTABLE_SUFFIX}")

if(WIN32)
  include_directories("${PROJECT_SOURCE_DIR}/src/qtwinmigrate")
  list(APPEND project_directories "qtwinmigrate")
endif()
list(APPEND project_directories "utilities")
list(APPEND project_directories "model_editor")
list(APPEND project_directories "bimserver")
list(APPEND project_directories "openstudio_lib")
list(APPEND project_directories "openstudio_app")

foreach(D ${project_directories})
  add_subdirectory(src/${D})
endforeach()

add_subdirectory("ruby")

###############################################################################
#                        E X P O R T    T A R G E T S                         #
###############################################################################
# Export targets and generate OpenStudioCoreConfig.cmake

if(WIN32)
  list(APPEND all_lib_targets "qtwinmigrate")
endif()
list(APPEND all_lib_targets "openstudio_modeleditor")
list(APPEND all_lib_targets "openstudio_bimserver")
list(APPEND all_lib_targets "openstudio_lib")

list(APPEND all_exe_targets "OpenStudioApp")

if(UNIX AND NOT APPLE)
  foreach(targ ${all_lib_targets} ${all_exe_targets} ${ALL_TESTING_TARGETS})
    set_target_properties(${targ} PROPERTIES
      #ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_ARCHIVE_OUTPUT_DIRECTORY}"
      #LIBRARY_OUTPUT_DIRECTORY "${PROJECT_LIBRARY_OUTPUT_DIRECTORY}"
      #RUNTIME_OUTPUT_DIRECTORY "${PROJECT_RUNTIME_OUTPUT_DIRECTORY}"
      VERSION ${OpenStudioApplication_VERSION}
    )
  #target_include_directories(${targ} PUBLIC openstudio)
  endforeach()
endif()


##########################################################   S Y S T E M    L I B R A R I E S   ######################################################

# Add compiler-provided system runtime libraries
if( NOT UNIX )
  # Skip the call to install(PROGRAMS) so we can specify our own install rule (using the value of `CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS`)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)

  # Set to TRUE to install the Windows Universal CRT libraries for app-local deployment.
  # This is meaningful only with MSVC from Visual Studio 2015 or higher, which is our case
  #SET(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)

  include(InstallRequiredSystemLibraries)
  if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
    message(DEBUG "CMAKE_INSTALL_SYSTEM_RUNTIME_LIBs: ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}")
    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION "bin" COMPONENT Libraries)
  endif()
endif()

###############################################################################
#                              I N S T A L L E R                              #
###############################################################################

######### Install SDK dependencies here ################

#install(DIRECTORY "${openstudio_ROOT_DIR}/EnergyPlus" DESTINATION "." COMPONENT "OpenStudioApp" USE_SOURCE_PERMISSIONS)
install(DIRECTORY "${openstudio_ROOT_DIR}/Radiance" DESTINATION "." COMPONENT "OpenStudioApp" USE_SOURCE_PERMISSIONS)
install(DIRECTORY "${openstudio_ROOT_DIR}/Ruby" DESTINATION "." COMPONENT "OpenStudioApp" USE_SOURCE_PERMISSIONS)
install(DIRECTORY "${openstudio_ROOT_DIR}/EnergyPlus" DESTINATION "." COMPONENT "OpenStudioApp" USE_SOURCE_PERMISSIONS)
install(DIRECTORY "${openstudio_ROOT_DIR}/Examples" DESTINATION "." COMPONENT "OpenStudioApp" USE_SOURCE_PERMISSIONS)
# I don't see any point shipping libopenstudiolib in `lib/` when it's already installed in ./bin/ via fixup_bundle
#install(DIRECTORY "${openstudio_ROOT_DIR}/lib" DESTINATION "." COMPONENT "OpenStudioApp" USE_SOURCE_PERMISSIONS)

install(PROGRAMS ${os_cli_location} DESTINATION bin COMPONENT "CLI" RENAME "${os_cli_install_name}")

if(WIN32)
  install(PROGRAMS "${openstudio_ROOT_DIR}/bin/install_utility.exe" DESTINATION bin COMPONENT "CLI")
else()
  install(PROGRAMS "${openstudio_ROOT_DIR}/bin/install_utility" DESTINATION bin COMPONENT "CLI")
endif()

# Leaving it
set(CPACK_IFW_VERBOSE ON)

set(CPACK_PACKAGE_VENDOR "National Renewable Energy Laboratory")
# These are ok, even if we have a prerelease tag
set(CPACK_PACKAGE_VERSION_MAJOR ${OpenStudioApplication_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${OpenStudioApplication_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${OpenStudioApplication_VERSION_PATCH})

# CPACK_DEBIAN_PACKAGE_DESCRIPTION defaults to this one too. dpkg-deb -I xxx.deb will show this description
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenStudioApplication ${OpenStudioApplication_VERSION}, based on openstudio (core) ${OPENSTUDIO_VERSION}. OpenStudio is a cross-platform collection of software tools to support whole building energy modeling using EnergyPlus and advanced daylight analysis using Radiance")

# The actual package file name on disk
if(APPLE)
  include(CMake/TargetArch.cmake)
  target_architecture(TARGET_ARCH)

  # eg OpenStudioApplication-1.2.0-alpha+394bbe9f8d-macOS-10.15-x86_64 (future: OpenStudioApplication-1.2.0-alpha+394bbe9f8d-macOS-11.2-arm64)
  set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${OPENSTUDIOAPPLICATION_LONG_VERSION}-macOS${OSX_VERSION_MAJOR_MINOR}-${TARGET_ARCH}")
elseif(UNIX)
  # eg: OpenStudioApplication-1.2.0-alpha+394bbe9f8d-Ubuntu18.04
  set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${OPENSTUDIOAPPLICATION_LONG_VERSION}-${LSB_RELEASE_ID_SHORT}${LSB_RELEASE_VERSION_SHORT}")
else()
  # eg: OpenStudioApplication-1.2.0-alpha+394bbe9f8d-Windows
  set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${OPENSTUDIOAPPLICATION_LONG_VERSION}-${CMAKE_SYSTEM_NAME}")
endif()

set(CPACK_PACKAGE_CONTACT "openstudio@nrel.gov")

  # Install additional Documents, such as release notes
  install(FILES "${PROJECT_SOURCE_DIR}/QT_LICENSE_GPL_3_0.txt" DESTINATION .)
  install(FILES "${PROJECT_SOURCE_DIR}/QT_LICENSE_LGPL_3_0.txt" DESTINATION .)
  install(FILES "${PROJECT_SOURCE_DIR}/LICENSE.md" DESTINATION .)

if (APPLE)
  set(CPACK_IFW_TARGET_DIRECTORY /Applications/OpenStudioApplication-${OpenStudioApplication_VERSION}/)
elseif(UNIX)

  # For debug
  # set(CPACK_DEBIAN_PACKAGE_DEBUG ON)

  # Trusty (14.04) uses 3.0-0, all other including Xenial (16.04) and most importantly the supported one Bionic (18.04) use 3.0-0v5
  # Focal Fossa (20.04) uses libwxgtk3.0-gtk3-0v5, apparently need libxcb-xinerama0 as well
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxcb-xinerama0 (>= 1.13), libwxgtk3.0-0v5 (>= 3.0.0) | libwxgtk3.0-0 (>=3.0.0) | libwxgtk3.0-gtk3-0v5 (>=3.0.0)")

  # These two will set the .deb install path correctly
  set(CPACK_SET_DESTDIR ON)
  set(CPACK_INSTALL_PREFIX /usr/local/openstudioapplication-${OpenStudioApplication_VERSION})

  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "osc@openstudiocoalition.org")
  # Default the debian package name to include version to allow several versions to be installed concurrently instead of overwriting any existing one
  set(CPACK_DEBIAN_PACKAGE_NAME "openstudioapplication-${OpenStudioApplication_VERSION}") # Includes the prerelease tag if any
  set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://openstudiocoalition.org/")

  set(POSTINST_FILE "${PROJECT_BINARY_DIR}/postinst")
  set(POSTRM_FILE "${PROJECT_BINARY_DIR}/postrm")
  configure_file(${PROJECT_SOURCE_DIR}/debian/postinst.in ${POSTINST_FILE} @ONLY)
  configure_file(${PROJECT_SOURCE_DIR}/debian/postrm.in ${POSTRM_FILE} @ONLY)
  execute_process(COMMAND chmod 755 "${POSTINST_FILE}")
  execute_process(COMMAND chmod 755 "${POSTRM_FILE}")

  ## Add a symlink to the App: /usr/local/bin/OpenStudioApp should point to /usr/local/openstudioapplication-${OPENSTUDIOAPPLICATION_VERSION}/bin/OpenStudioApp
  ## Add an arbitrary link (broken) at build/OpenStudioApp (if build/ is your build folder) that already points to the **future** /usr/local/openstudioapplication-x.y.z/bin/OpenStudioApp
  #execute_process(COMMAND ln -sf ${CPACK_INSTALL_PREFIX}/bin/OpenStudioApp ${CMAKE_CURRENT_BINARY_DIR}/OpenStudioApp)
  ## Plus a versioned one
  #execute_process(COMMAND ln -sf ${CPACK_INSTALL_PREFIX}/bin/OpenStudioApp ${CMAKE_CURRENT_BINARY_DIR}/OpenStudioApp-${OPENSTUDIOAPPLICATION_VERSION})
  ## Have these links be installed with the .deb package in /usr/local/bin
  #install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenStudioApp DESTINATION /usr/local/bin COMPONENT OpenStudioApp)
  #install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OpenStudioApp-${OPENSTUDIOAPPLICATION_VERSION} DESTINATION /usr/local/bin COMPONENT OpenStudioApp)

  ## Add an arbitrary link (broken) at build/openstudio (if build/ is your build folder) that already points to the **future** /usr/local/openstudioapplication-x.y.z/bin/openstudio
  #execute_process(COMMAND ln -sf ${CPACK_INSTALL_PREFIX}/bin/openstudio ${CMAKE_CURRENT_BINARY_DIR}/openstudio)
  ## Plus a versioned one
  #execute_process(COMMAND ln -sf ${CPACK_INSTALL_PREFIX}/bin/openstudio ${CMAKE_CURRENT_BINARY_DIR}/openstudio-${OPENSTUDIO_VERSION})
  ## Have this link be installed with the .deb package in /usr/local/bin
  #install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openstudio DESTINATION /usr/local/bin COMPONENT CLI)
  #install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openstudio-${OpenStudio_VERSION} DESTINATION /usr/local/bin COMPONENT CLI)


  # Create a proper App with file associations

  # Install the .desktop manifest (allows the App to be seen in the Dash and adding to the dock, and to map it a mimetype)
  # install(FILES "${PROJECT_SOURCE_DIR}/src/openstudio_app/openstudioapp.desktop" DESTINATION /usr/share/applications)
  install(FILES "${PROJECT_SOURCE_DIR}/src/openstudio_app/openstudioapp.desktop" DESTINATION Temp/)

  # Install the XML mime info
  # install(FILES "${PROJECT_SOURCE_DIR}/src/openstudio_app/x-openstudio.xml" DESTINATION /usr/share/mime/application)
  install(FILES "${PROJECT_SOURCE_DIR}/src/openstudio_app/x-openstudio.xml" DESTINATION Temp/)

  set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CPACK_DEBIAN_BIN_PACKAGE_CONTROL_EXTRA};${POSTINST_FILE};${POSTRM_FILE}")

  ## try to set up the menu system
  # Problem is the use of "sudo". Perhaps we should copy icons and manifest to $HOME/.local/share/...
  #find_program(XDG-MIME_EXECUTABLE xdg-mime)
  #find_program(XDG-DESKTOP-MENU_EXECUTABLE xdg-desktop-menu)
  #find_program(UPDATE-MIME-DATABASE-EXECUTABLE update-mime-database)
  #find_program(GTK-UPDATE-ICON-CACHE-EXECUTABLE gtk-update-icon-cache)


  ## Install the Desktop Menu
  ## xdg-desktop-menu install --novendor /usr/share/applications/openstudioapp.desktop
  #install(CODE "execute_process(COMMAND ${XDG-DESKTOP-MENU_EXECUTABLE} install --novendor --mode system /usr/share/applications/openstudioapp.desktop)")

  ## Register the mime description: this places it in /usr/share/mime/packages/
  ## sudo xdg-mime install --novendor --mode system x-openstudio.xml
  #install(CODE "execute_process(COMMAND sudo ${MIME_EXECUTABLE} install --novendor --mode system /usr/share/mime/application/x-openstudio.xml)")
  #install(CODE "execute_process(COMMAND echo COUCOU)")

  ## Register the application launcher with the registered mimetype
  ## sudo xdg-mime default openstudio.desktop application/x-openstudio
  #install(CODE "execute_process(COMMAND sudo ${MIME_EXECUTABLE} default /usr/share/applications/openstudio.desktop application/x-openstudio)")


  ## Update the mime cache and icon cache
  ## sudo update-mime-database /usr/share/mime
  #install(CODE "execute_process(COMMAND sudo ${UPDATE-MIME-DATABASE-EXECUTABLE} /usr/share/mime)")
  ## sudo gtk-update-icon-cache /usr/share/icons/hicolor/ -f
  #install(CODE "execute_process(COMMAND ${GTK-UPDATE-ICON-CACHE-EXECUTABLE} /usr/share/icons/hicolor/ -f)")



elseif(WIN32)
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "openstudioapplication-${OpenStudioApplication_VERSION}")
  set(CPACK_IFW_TARGET_DIRECTORY "C:/${CPACK_PACKAGE_INSTALL_DIRECTORY}")
  set(CPACK_BINARY_IFW ON CACHE BOOL "Enable to build IFW packages")
  set(CPACK_BINARY_NSIS OFF CACHE BOOL "Enable to build NSIS packages")
endif()

######################################################################################################################################################
#                                                    P A C K A G I N G   &   C O M P O N E N T S                                                     #
######################################################################################################################################################

# Careful: the position (and what you include) matters a lot!
include(CPack)
include(CPackIFW)
# include(CPackComponent)

# Note: If you ever need to debug a CPack Package Error in MSVC, right-click on "PACKAGE" target, click Properties
# and in the Build Events > Post Build Event, edit the command that calls cpack to add `--verbose --debug`:
# eg: `"C:\Program Files\CMake\bin\cpack.exe" -C $(Configuration) --config ./CPackConfig.cmake --verbose --debug`


cpack_add_component(Resources
  DISPLAY_NAME "Application Resources"
  DESCRIPTION "Resource Files used by the OpenStudio Application"
)

cpack_add_component(OpenStudioApp
  DISPLAY_NAME "OpenStudio Application"
  DESCRIPTION "OpenStudio Application"
)

cpack_ifw_configure_component(Resources
  REQUIRES_ADMIN_RIGHTS
)

cpack_ifw_configure_component(OpenStudioApp
  DEPENDS Resources CLI
  SCRIPT src/openstudio_app/install_operations.qs
  REQUIRES_ADMIN_RIGHTS
)

cpack_add_component(Libraries
  DISPLAY_NAME "Install required system libraries"
  DESCRIPTION  "Install compiler-provided system runtime libraries, and Windows Universal CRT libraries for app-local deployment"
  REQUIRED
  HIDDEN
)

add_custom_target(PACKAGE_DEBUG
  COMMAND ${CMAKE_CPACK_COMMAND} --debug --verbose --config CPackConfig.cmake
  COMMENTS "Build package with debug and verbose output"
)
