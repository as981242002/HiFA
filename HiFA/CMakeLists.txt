cmake_minimum_required(VERSION 2.8)

project(HiFA)
add_subdirectory(Base)
add_subdirectory(Net)
add_executable(${PROJECT_NAME} "main.cpp")

target_link_libraries(${PROJECT_NAME} HiFA_base HiFA_net "/usr/local/lib/libgtest.a" "-lpthread")
