# from here:
#
# https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md

function(set_project_warnings project_name)
  option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" Off)

  set(MSVC_WARNINGS
      /W3 # Baseline reasonable warnings
      /w14242 # 'identifier': conversion from 'type1' to 'type1', possible loss of data
      /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
      /w14263 # 'function': member function does not override any base class virtual member function
      /w14287 # 'operator': unsigned/negative constant mismatch
      /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside
              # the for-loop scope
      /w14296 # 'operator': expression is always 'boolean_value'
      /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
      /w14545 # expression before comma evaluates to a function which is missing an argument list
      /w14546 # function call before comma missing argument list
      /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
      /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
      /w14555 # expression has no effect; expected expression with side- effect
      /w14619 # pragma warning: there is no warning number 'number'
      /w14640 # Enable warning on thread un-safe static member initialization
      /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
      /w14905 # wide string literal cast to 'LPSTR'
      /w14906 # string literal cast to 'LPWSTR'
      /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied

      # disable:
      /wd4018  # 'token' : signed/unsigned mismatch
      /wd4242  # 'identifier' : conversion from 'type1' to 'type2', possible loss of data
      /wd4244  # 'conversion' conversion from 'type1' to 'type2', possible loss of data
      /wd4267  # 'var' : conversion from 'size_t' to 'type', possible loss of data
      /wd4305  # 'context' : truncation from 'type1' to 'type2'
      /wd26451 # Arithmetic overflow: Using operator 'operator' on a size-a byte value and then casting the result to a size-b byte value. Cast the value to the wider type before calling operator 'operator' to avoid overflow

      /permissive- # standards conformance mode for MSVC compiler.
  )

  set(CLANG_WARNINGS
      -Wall
      -Wextra # reasonable and standard
      -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps
                         # catch hard to track down memory errors

      -Wno-unknown-pragmas
      -Wno-unused-parameter
      -Wno-unused-function
      -Wno-unused-variable
  )

  if(WARNINGS_AS_ERRORS)
    set(CLANG_WARNINGS ${CLANG_WARNINGS} -Werror)
    set(MSVC_WARNINGS ${MSVC_WARNINGS} /WX)
  endif()

  set(GCC_WARNINGS
      ${CLANG_WARNINGS}
      -Wduplicated-cond # warn if if / else chain has duplicated conditions
      -Wduplicated-branches # warn if if / else branches have duplicated code
      -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
  )

  if(MSVC)
    set(PROJECT_WARNINGS ${MSVC_WARNINGS})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(PROJECT_WARNINGS ${CLANG_WARNINGS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_WARNINGS ${GCC_WARNINGS})
  else()
    message(AUTHOR_WARNING "No compiler warnings set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
  endif()

  option(ENABLE_WARNINGS "Enable compiler warnings" ON)
  if (ENABLE_WARNINGS)
    target_compile_options(${project_name} PRIVATE ${PROJECT_WARNINGS})
  endif()

endfunction()
