foreach(required PUBLISH_SCRIPT TEST_ROOT)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "release.rehearsal_delete: ${required} is required")
  endif()
endforeach()
find_program(RELEASE_DELETE_SH sh REQUIRED)

set(fake_bin "${TEST_ROOT}/bin")
file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${fake_bin}")
file(WRITE "${fake_bin}/gh" [=[#!/bin/sh
case "$*" in
  *"-X DELETE repos/owner/repo/git/refs/tags/"*) : > "$DELETE_MARKER"; exit 0 ;;
  *"repos/owner/repo/git/ref/tags/"*)
    count=$(cat "$COUNTER" 2>/dev/null || printf 0)
    count=$((count + 1))
    printf '%s\n' "$count" > "$COUNTER"
    if [ "$MODE" = api_error ]; then echo "HTTP/2.0 500 Internal Server Error" >&2; exit 1; fi
    [ "$MODE" = persistent ] || [ "$count" -le 2 ] && exit 0
    echo "HTTP/2.0 404 Not Found" >&2
    exit 1 ;;
  *"-X DELETE repos/owner/repo/releases/"*) : > "$RELEASE_ID_DELETE_MARKER"; exit 0 ;;
  *"release delete rehearsal-"*) : > "$RELEASE_DELETE_MARKER"; exit 0 ;;
  *"repos/owner/repo/releases/42"*)
    count=$(cat "$RELEASE_ID_COUNTER" 2>/dev/null || printf 0)
    count=$((count + 1))
    printf '%s\n' "$count" > "$RELEASE_ID_COUNTER"
    if [ "$MODE" = release_error ]; then echo "HTTP/2.0 500 Internal Server Error" >&2; exit 1; fi
    echo "HTTP/2.0 404 Not Found" >&2
    exit 1 ;;
  *"repos/owner/repo/releases/tags/"*)
    count=$(cat "$RELEASE_TAG_COUNTER" 2>/dev/null || printf 0)
    count=$((count + 1))
    printf '%s\n' "$count" > "$RELEASE_TAG_COUNTER"
    if [ "$MODE" = release_tag_error ]; then echo "HTTP/2.0 500 Internal Server Error" >&2; exit 1; fi
    echo "HTTP/2.0 404 Not Found" >&2
    exit 1 ;;
esac
exit 0
]=])
file(WRITE "${fake_bin}/sleep" "#!/bin/sh\nexit 0\n")
file(CHMOD "${fake_bin}/gh" "${fake_bin}/sleep"
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

function(check_scenario mode expected_result expected_queries)
  set(counter "${TEST_ROOT}/${mode}.tag_count")
  set(release_id_counter "${TEST_ROOT}/${mode}.release_id_count")
  set(release_tag_counter "${TEST_ROOT}/${mode}.release_tag_count")
  set(delete_marker "${TEST_ROOT}/${mode}.tag_delete")
  set(release_id_delete_marker "${TEST_ROOT}/${mode}.release_id_delete")
  set(release_delete_marker "${TEST_ROOT}/${mode}.release_delete")
  set(release_args "rehearsal-delete;rehearsal-45000000000-1")
  if(NOT mode STREQUAL "no_id")
    list(APPEND release_args 42)
  endif()
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
      "PATH=${fake_bin}:$ENV{PATH}" "MODE=${mode}" "COUNTER=${counter}"
      "RELEASE_ID_COUNTER=${release_id_counter}" "RELEASE_TAG_COUNTER=${release_tag_counter}"
      "DELETE_MARKER=${delete_marker}" "RELEASE_ID_DELETE_MARKER=${release_id_delete_marker}"
      "RELEASE_DELETE_MARKER=${release_delete_marker}"
      GITHUB_REPOSITORY=owner/repo "${RELEASE_DELETE_SH}" "${PUBLISH_SCRIPT}" ${release_args}
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(mode STREQUAL "release_error")
    set(expected_error "release 42 still exists or deletion could not be confirmed")
  elseif(mode STREQUAL "release_tag_error")
    set(expected_error "release rehearsal-45000000000-1 still exists or deletion could not be confirmed")
  else()
    set(expected_error "tag rehearsal-45000000000-1 still exists or deletion could not be confirmed")
  endif()
  if(expected_result EQUAL 0)
    if(NOT result EQUAL 0)
      message(FATAL_ERROR "release.rehearsal_delete: retry did not recover: ${error}")
    endif()
  elseif(result EQUAL 0 OR NOT error MATCHES "${expected_error}")
    message(FATAL_ERROR "release.rehearsal_delete: ${mode} did not fail closed: ${output}${error}")
  endif()
  if(mode STREQUAL "release_error")
    file(READ "${release_id_counter}" queries)
    if(NOT queries STREQUAL "10\n" OR EXISTS "${release_tag_counter}" OR EXISTS "${counter}"
        OR NOT EXISTS "${release_id_delete_marker}" OR EXISTS "${delete_marker}")
      message(FATAL_ERROR "release.rehearsal_delete: release API failure did not stop cleanup")
    endif()
  elseif(mode STREQUAL "release_tag_error")
    file(READ "${release_id_counter}" id_queries)
    file(READ "${release_tag_counter}" tag_queries)
    if(NOT id_queries STREQUAL "1\n" OR NOT tag_queries STREQUAL "10\n" OR EXISTS "${counter}"
        OR NOT EXISTS "${release_id_delete_marker}" OR EXISTS "${delete_marker}")
      message(FATAL_ERROR "release.rehearsal_delete: release-by-tag API failure did not stop cleanup")
    endif()
  elseif(mode STREQUAL "no_id")
    file(READ "${release_tag_counter}" release_queries)
    if(NOT release_queries STREQUAL "1\n" OR EXISTS "${release_id_counter}"
        OR EXISTS "${release_id_delete_marker}"
        OR NOT EXISTS "${release_delete_marker}" OR NOT EXISTS "${delete_marker}")
      message(FATAL_ERROR "release.rehearsal_delete: tag-only deletion was not checked")
    endif()
  else()
    file(READ "${counter}" queries)
    file(READ "${release_id_counter}" id_queries)
    file(READ "${release_tag_counter}" release_queries)
    if(NOT queries STREQUAL "${expected_queries}\n" OR NOT id_queries STREQUAL "1\n"
        OR NOT release_queries STREQUAL "1\n" OR NOT EXISTS "${release_id_delete_marker}"
        OR NOT EXISTS "${delete_marker}"
        OR EXISTS "${release_delete_marker}")
      message(FATAL_ERROR "release.rehearsal_delete: ${mode} did not verify each deletion")
    endif()
  endif()
endfunction()

check_scenario(retry 0 3)
check_scenario(no_id 0 3)
check_scenario(persistent 1 10)
check_scenario(api_error 1 10)
check_scenario(release_error 1 unused)
check_scenario(release_tag_error 1 unused)
