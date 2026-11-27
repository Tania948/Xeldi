set(XELDI_TESTS test_canvas2d test_animation test_history test_voxel_grid test_engine3d test_format test_editor)
foreach(t ${XELDI_TESTS})
  add_executable(${t} ${t}.cpp)
  target_link_libraries(${t} PRIVATE xeldi_engine)
  add_test(NAME ${t} COMMAND ${t} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

if(XELDI_BUILD_APP)
  # Prueba de humo de la interfaz (plataforma offscreen, sin pantalla).
  add_executable(test_ui_smoke test_ui_smoke.cpp)
  target_link_libraries(test_ui_smoke PRIVATE xeldi_ui)
  add_test(NAME test_ui_smoke COMMAND test_ui_smoke)
  set_tests_properties(test_ui_smoke PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
