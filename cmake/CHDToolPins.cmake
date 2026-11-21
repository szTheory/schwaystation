file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../pins.txt" schway__chdtool_rows
  REGEX "^libchdman-rs ")
list(LENGTH schway__chdtool_rows schway__chdtool_count)
if(NOT schway__chdtool_count EQUAL 1)
  message(FATAL_ERROR "CHD tool: pins.txt needs exactly one libchdman-rs row")
endif()
string(REPLACE " " ";" schway__chdtool_row "${schway__chdtool_rows}")
list(GET schway__chdtool_row 1 schway__chdtool_commit)
list(GET schway__chdtool_row 2 schway__chdtool_url)
list(GET schway__chdtool_row 3 schway__chdtool_sha256)
if(NOT schway__chdtool_commit STREQUAL "7f71bdf3d4099dc38bded49c6745e453005d6e41"
    OR NOT schway__chdtool_url MATCHES "/archive/${schway__chdtool_commit}[.]tar[.]gz$")
  message(FATAL_ERROR "CHD tool: pin URL or commit does not match the audited source")
endif()
