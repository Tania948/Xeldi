cmake_minimum_required(VERSION 3.22)
project(Xeldi VERSION 0.1.0 DESCRIPTION "Editor de pixel art 2D y construccion 3D en voxeles" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

option(XELDI_BUILD_APP   "Construir la interfaz de escritorio (Qt 6)" ON)
option(XELDI_BUILD_TESTS "Construir las pruebas" ON)

include(GNUInstallDirs)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Motor sin dependencias de Qt (core, engine2d, engine3d, io).
add_subdirectory(src)

if(XELDI_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(XELDI_BUILD_APP)
  install(FILES packaging/linux/io.github.tania948.Xeldi.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES packaging/linux/io.github.tania948.Xeldi.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
  install(FILES packaging/linux/xeldi-mime.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages RENAME xeldi.xml)
  foreach(size 16 32 48 64 128 256 512)
    install(FILES assets/icons/hicolor/${size}x${size}/apps/xeldi.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps)
  endforeach()
  install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()

# Empaquetado .deb (Ubuntu/Debian) con CPack.
set(CPACK_PACKAGE_NAME "xeldi")
set(CPACK_PACKAGE_VENDOR "Tania948")
set(CPACK_PACKAGE_CONTACT "Tania948 <https://github.com/Tania948/xeldi>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Editor de pixel art 2D con construccion 3D en voxeles")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Tania948/xeldi")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# Plugins de plataforma (X11/Wayland): no los detecta shlibdeps.
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-qpa-plugins, shared-mime-info")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "qt6-wayland")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/packaging/debian/postinst;${CMAKE_SOURCE_DIR}/packaging/debian/postrm")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
include(CPack)
