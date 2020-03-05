find_program(CURL curl)
if (CURL)
    add_custom_target(iso_paper
        COMMAND
            pwd
        COMMAND
            ${CURL} -k https://api.csswg.org/bikeshed/ -F file=@${CMAKE_CURRENT_SOURCE_DIR}/p1950.md -F force=1 > p1950r0.html
        BYPRODUCTS
            p1950r0.html
        COMMENT
            "Generating ISO Paper p1950r0"
    )
endif ()
