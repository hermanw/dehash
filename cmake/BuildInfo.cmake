find_package(Git)
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%as-%h RESULT_VARIABLE res_var OUTPUT_VARIABLE GIT_COM_ID )
    if( NOT ${res_var} EQUAL 0 )
        set( GIT_COMMIT_ID "git commit id unknown")
        message( WARNING "Git failed (not a repo, or no tags). Build will not contain git revision info." )
    endif()
    string( REPLACE "\n" "" GIT_COMMIT_ID ${GIT_COM_ID} )
else()
    set( GIT_COMMIT_ID "unknown (git not found!)")
    message( WARNING "Git not found. Build will not contain git revision info." )
endif()

SET(BUILDINFO_FILENAME "buildinfo.h")

# TODO: windows suppport
execute_process(COMMAND date RESULT_VARIABLE res_var OUTPUT_VARIABLE BUILDINFO_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND hostname -f RESULT_VARIABLE res_var OUTPUT_VARIABLE BUILDINFO_HOSTNAME OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND whoami RESULT_VARIABLE res_var OUTPUT_VARIABLE BUILDINFO_USERNAME OUTPUT_STRIP_TRAILING_WHITESPACE)


set( vstring "// ${BUILDINFO_FILENAME} - written by cmake. changes will be lost!\n"
    "#ifndef FEATUREGRAPH_BUILDINFO_H\n"
    "#define FEATUREGRAPH_BUILDINFO_H\n"
    "\n"
    "namespace BuildInfo {\n"
    "  static const char * version_string = \"${GIT_COMMIT_ID}\"\;\n"
    "  static const char * date = \"${BUILDINFO_DATE}\"\;\n"
    "  static const char * hostname = \"${BUILDINFO_HOSTNAME}\"\;\n"
    "  static const char * username = \"${BUILDINFO_USERNAME}\"\;\n"
    "  static const char * type = \"${CMAKE_BUILD_TYPE}\"\;\n"
    "};\n"
    "#endif // FEATUREGRAPH_BUILDINFO_H\n"
)

file(WRITE src/buildinfo.h ${vstring} )
