add_executable(test_mkvtag test_mkvtag.c)
target_link_libraries(test_mkvtag PRIVATE mkvtag)
target_include_directories(test_mkvtag PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_mkvtag COMMAND test_mkvtag)
