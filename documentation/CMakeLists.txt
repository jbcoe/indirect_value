find_program(CURL curl)
if (CURL)
    add_custom_target(iso_paper
        COMMAND
            pwd
        COMMAND
            ${CURL} -k https://api.csswg.org/bikeshed/ -F file=@${CMAKE_CURRENT_SOURCE_DIR}/p1950.md -F force=1 > P1950r2.html
        BYPRODUCTS
            P1950r2.html
        COMMENT
            "Generating ISO Paper P1950 revision 2"
    )
endif ()
