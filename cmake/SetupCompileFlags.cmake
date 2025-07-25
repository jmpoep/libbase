function(SETUP_COMPILE_FLAGS)
  set(DEFINES "")

  string(REGEX MATCH "[0-9]+" COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})

  if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    string(APPEND DEFINES "LIBBASE_IS_LINUX")
  elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    string(APPEND DEFINES "LIBBASE_IS_WINDOWS")
  elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
    string(APPEND DEFINES "LIBBASE_IS_MACOS")
  endif()

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      # -Wold-style-cast
      set(WARNINGS "-Wall;-Wextra;-Werror;-Wunreachable-code")

      if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(WARNINGS "${WARNINGS};-Wpedantic;-Wshadow;-Wno-gnu-zero-variadic-macro-arguments;-Wno-c++98-compat;-Wno-c++98-compat-pedantic;-Wno-exit-time-destructors;-Wno-global-constructors;-Wno-missing-prototypes;-Wno-ctad-maybe-unsupported;-Wno-switch-default;-Wno-extra-semi-stmt;-Wno-switch-enum")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.0)
          set(WARNINGS "${WARNINGS};-Wno-unsafe-buffer-usage")
        endif()
      else()
        set(WARNINGS "${WARNINGS};-Wshadow=local")

        # GCC 7.x and older doesn't handle variadic macros that well, so enable
        # pedanting warnings only on newer versions to avoid the:
        # > ISO C++11 requires at least one argument for the "..." in a variadic
        # > macro
        # error
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.0)
          set(WARNINGS "${WARNINGS};-Wpedantic")
        endif()
      endif()
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
      set(WARNINGS "/W4;/WX;/EHsc;/permissive-;/W34996;/W4244")
  endif()

  if(LIBBASE_BUILD_TESTS AND LIBBASE_CODE_COVERAGE)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
      set(COVERAGE_COMPILE_FLAGS ";-g;-fno-inline;-fno-elide-constructors;-fno-inline-small-functions;-fno-default-inline;-fprofile-arcs;-ftest-coverage;-fprofile-update=atomic")
      set(COVERAGE_LINK_FLAGS "-lgcov;-fprofile-update=atomic")
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      set(COVERAGE_COMPILE_FLAGS ";-fprofile-instr-generate;-fcoverage-mapping")
      set(COVERAGE_LINK_FLAGS "-fprofile-instr-generate;-fcoverage-mapping")
    else()
      message(FATAL_ERROR "Code coverage supported only with GCC and Clang compilers")
    endif()
  endif()

  find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-${COMPILER_VERSION})
  if(LIBBASE_CLANG_TIDY AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND CLANG_TIDY_EXE)
    set(CLANG_TIDY_PROPERTIES "CXX_CLANG_TIDY;${CLANG_TIDY_EXE}")
  endif()

  if(LIBBASE_BUILD_ASAN AND LIBBASE_BUILD_TSAN)
    message(FATAL_ERROR "Cann't use ASAN and TSAN within same build")
  endif()

  if(LIBBASE_BUILD_ASAN)
    set(SANITIZE_FLAGS ";-fsanitize=address,undefined")
  elseif(LIBBASE_BUILD_TSAN)
    set(SANITIZE_FLAGS ";-fsanitize=thread")
  endif()

  if(NOT CONFIGURED_ONCE)
    set(
      LIBBASE_COMPILE_FLAGS
      "${WARNINGS}${COVERAGE_COMPILE_FLAGS}${SANITIZE_FLAGS}"
      CACHE STRING "Flags used by the compiler to build targets"
      FORCE)
    set(
      LIBBASE_LINK_FLAGS
      "${COVERAGE_LINK_FLAGS}${SANITIZE_FLAGS}"
      CACHE STRING "Flags used by the linker to link targets"
      FORCE)
    set(
      LIBBASE_DEFINES
      "${DEFINES}"
      CACHE STRING "Preprocessor defines"
      FORCE
    )
    set(
      LIBBASE_OPT_CLANG_TIDY_PROPERTIES
      "${CLANG_TIDY_PROPERTIES}"
      CACHE STRING "Properties used to enable clang-tidy when building targets"
      FORCE)
  endif()
endfunction()


setup_compile_flags()


function(libbase_configure_library_target target_name)
  target_compile_features(${target_name}
    PUBLIC
      cxx_std_17
  )

  target_compile_options(${target_name}
    PRIVATE
      ${LIBBASE_COMPILE_FLAGS}
  )

  target_compile_definitions(${target_name}
    PUBLIC
      ${LIBBASE_DEFINES}
  )

  set_target_properties(${target_name}
    PROPERTIES
      CXX_EXTENSIONS ON
      ${LIBBASE_OPT_CLANG_TIDY_PROPERTIES}
  )

  target_include_directories(${target_name} PUBLIC
      $<BUILD_INTERFACE:${libbase_SOURCE_DIR}/src/>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libbase>
  )

  target_link_libraries(${target_name}
    PUBLIC
      ${LIBBASE_LINK_FLAGS}
      Threads::Threads
      glog::glog
  )
endfunction()
