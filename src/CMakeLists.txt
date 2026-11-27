find_package(ZLIB REQUIRED)
find_package(nlohmann_json 3.10 REQUIRED)

# ---------------------------------------------------------------------------
# xeldi_engine: todo el modelo y los algoritmos, SIN Qt.
# Esta libreria es la que se reutilizara en tablets / otros frontends.
# ---------------------------------------------------------------------------
add_library(xeldi_engine STATIC
  core/Color.cpp
  core/Image.cpp
  core/Palette.cpp
  core/Timeline.cpp
  core/VoxelGrid.cpp
  core/Workplane.cpp
  core/Document.cpp
  core/Command.cpp
  core/Commands.cpp
  core/Actions.cpp
  engine2d/Compositor.cpp
  engine2d/BrushTools.cpp
  engine2d/OnionSkin.cpp
  engine2d/AnimationPlayer.cpp
  engine3d/CameraMath.cpp
  engine3d/Picking.cpp
  engine3d/SoftwareRenderer.cpp
  engine3d/Voxelize.cpp
  engine3d/ViewProjection.cpp
  io/Binary.cpp
  io/Png.cpp
  io/XeldiFormat.cpp
  io/MeshExporter.cpp
  io/SpritesheetExporter.cpp
  editor/Editor.cpp
)
target_include_directories(xeldi_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(xeldi_engine PUBLIC ZLIB::ZLIB PRIVATE nlohmann_json::nlohmann_json)
target_compile_definitions(xeldi_engine PUBLIC XELDI_VERSION="${PROJECT_VERSION}")

if(XELDI_BUILD_APP)
  add_subdirectory(app)
endif()
