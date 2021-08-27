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


set( vstring "// ${BUILDINFO_FILENAME} - written by cmake. changes will be lost!\n"
    "#ifndef FEATUREGRAPH_BUILDINFO_H\n"
    "#define FEATUREGRAPH_BUILDINFO_H\n"
    "\n"
    "namespace BuildInfo {\n"
    "  static const char * version_string = \"${GIT_COMMIT_ID}\"\;\n"
    "};\n"
    "#endif // FEATUREGRAPH_BUILDINFO_H\n"
)

file(WRITE src/buildinfo.h ${vstring} )
