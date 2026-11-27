find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)
set(CMAKE_AUTOMOC ON)

# Interfaz de escritorio como libreria (la usa el ejecutable y la prueba de humo).
add_library(xeldi_ui STATIC
  Theme.cpp
  QtBridge.cpp
  CanvasView2D.cpp
  Viewport3D.cpp
  LayersPanel.cpp
  TimelinePanel.cpp
  PalettePanel.cpp
  PreviewPanel.cpp
  ScenePanel.cpp
  MainWindow.cpp
  ExampleProject.cpp
)
target_link_libraries(xeldi_ui PUBLIC xeldi_engine Qt6::Widgets)

qt_add_executable(xeldi main.cpp)

file(GLOB XELDI_UI_ICONS RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/assets/ui/*.png)
file(GLOB XELDI_MASCOT RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/assets/mascot/*.png)
qt_add_resources(xeldi_ui "xeldi_assets"
  PREFIX "/"
  BASE ${PROJECT_SOURCE_DIR}/assets
  FILES
    ${PROJECT_SOURCE_DIR}/assets/icons/hicolor/256x256/apps/xeldi.png
    ${PROJECT_SOURCE_DIR}/assets/icons/hicolor/32x32/apps/xeldi.png
)
foreach(f ${XELDI_UI_ICONS} ${XELDI_MASCOT})
  list(APPEND XELDI_RES_FILES ${PROJECT_SOURCE_DIR}/${f})
endforeach()
qt_add_resources(xeldi_ui "xeldi_art" PREFIX "/" BASE ${PROJECT_SOURCE_DIR}/assets FILES ${XELDI_RES_FILES})

target_link_libraries(xeldi PRIVATE xeldi_ui)
set_target_properties(xeldi PROPERTIES OUTPUT_NAME xeldi)
install(TARGETS xeldi RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
