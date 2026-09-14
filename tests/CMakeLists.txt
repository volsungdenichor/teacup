include(FetchContent)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(teacup_tests
    test_canvas.cpp
    test_color.cpp
    test_input_decoder.cpp
    test_layout.cpp
    test_list.cpp
    test_table.cpp
    test_spinner_tabs.cpp
    test_text.cpp
    test_text_input.cpp
    test_widget_state_view.cpp
    test_viewport.cpp
)
target_link_libraries(teacup_tests PRIVATE teacup::teacup Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(teacup_tests)
