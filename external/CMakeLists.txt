# external/CMakeLists.txt
#
# Placeholder for vendored/third-party dependencies that aren't available
# via find_package() on all target platforms.
#
# Nothing is vendored yet — Eigen3, Boost, GTest, and pybind11 are currently
# resolved via find_package()/vcpkg in the top-level CMakeLists.txt and CI
# workflow. If a dependency needs to be vendored in future (e.g. via
# FetchContent), add it here:
#
#   include(FetchContent)
#   FetchContent_Declare(
#       some_lib
#       GIT_REPOSITORY https://github.com/example/some_lib.git
#       GIT_TAG        v1.2.3
#   )
#   FetchContent_MakeAvailable(some_lib)
