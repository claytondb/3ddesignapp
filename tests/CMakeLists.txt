# Tests

find_package(Catch2 3 QUIET)

if(Catch2_FOUND)
    add_executable(dc3d_tests
        test_main.cpp
    )
    
    target_link_libraries(dc3d_tests PRIVATE
        Catch2::Catch2WithMain
        dc3d_core
        dc3d_geometry
        dc3d_io
    )
    
    include(CTest)
    include(Catch)
    catch_discover_tests(dc3d_tests)
else()
    message(STATUS "Catch2 not found - tests disabled")
endif()
