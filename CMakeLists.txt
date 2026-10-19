cmake_minimum_required(VERSION 3.28)
project(enshrouded_shared_xp VERSION 1.1.0 LANGUAGES CXX ASM_MASM)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Static CRT: the Wine prefix of the server image needs no extra runtime.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
set(SAFETYHOOK_FETCH_ZYDIS ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    safetyhook
    GIT_REPOSITORY https://github.com/cursey/safetyhook.git
    GIT_TAG v0.7.0
)
FetchContent_MakeAvailable(safetyhook)

add_library(xpshare_core STATIC src/ledger.cpp src/scan.cpp)
target_include_directories(xpshare_core PUBLIC src)

# The DLL is named powrprof.dll because the server loads it in place of the real one.
add_library(powrprof SHARED
    src/main.cpp
    src/config.cpp
    src/log.cpp
    src/hooks.cpp
    src/memory.cpp
    src/ecs.cpp
    src/proxy.cpp
    src/powrprof_stubs.asm
    src/powrprof.def
)
target_link_libraries(powrprof PRIVATE xpshare_core safetyhook::safetyhook)
target_compile_definitions(powrprof PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE
    XPSHARE_VERSION="${PROJECT_VERSION}")
target_compile_options(powrprof PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4 /permissive->)

# Offline check against an enshrouded_server.exe on disk: would the DLL hook this build?
add_executable(xpshare_selftest tools/selftest.cpp)
target_link_libraries(xpshare_selftest PRIVATE xpshare_core)
target_compile_definitions(xpshare_selftest PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

add_executable(ledger_test tests/ledger_test.cpp)
target_link_libraries(ledger_test PRIVATE xpshare_core)

enable_testing()
add_test(NAME ledger COMMAND ledger_test)
