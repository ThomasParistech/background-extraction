add_executable(capsules_solver ${COMMON_SOURCES} capsules_solver_app.cpp)
target_link_libraries(capsules_solver ${OpenCV_LIBS} ${Boost_LIBRARIES})