cmake_minimum_required(VERSION 3.13)

add_executable(test_koto_loopen
        test_loopen.cpp
)

target_link_libraries(test_koto_loopen PRIVATE
        GTest::gtest_main
        kotoamatsukami_static
)

gtest_discover_tests(test_koto_loopen)

add_executable(test_koto_passes
        test_passes.cpp
)

target_link_libraries(test_koto_passes PRIVATE
        GTest::gtest_main
        kotoamatsukami_static
)

gtest_discover_tests(test_koto_passes)
