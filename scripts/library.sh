#!/bin/sh
set -eu

fail() { printf '%s\n' "library: $1" >&2; exit 1; }
project_root=$(CDPATH= cd "$(dirname "$0")/.." && pwd -P)

schway__library_absolute_path() {
    case "$1" in
        /*|[A-Za-z]:/*) printf '%s\n' "$1" ;;
        *) return 1 ;;
    esac
}

schway__library_absolute_directory() {
    schway__directory=$(schway__library_absolute_path "$1") || return 1
    (CDPATH= cd "$schway__directory" 2>/dev/null && pwd -P)
}

schway__library_absolute_file() {
    schway__file=$(schway__library_absolute_path "$1") || return 1
    schway__directory=$(schway__library_absolute_directory "$(dirname "$schway__file")") || return 1
    printf '%s/%s\n' "$schway__directory" "$(basename "$schway__file")"
}

test_root= test_config= test_ledger= test_dat= test_probe_config= test_runner=
test_mode=0
while [ "$#" -gt 0 ]; do
    case "$1" in
        --test-root) test_root=${2-}; test_mode=1 ;;
        --test-config) test_config=${2-}; test_mode=1 ;;
        --test-ledger) test_ledger=${2-}; test_mode=1 ;;
        --test-dat) test_dat=${2-}; test_mode=1 ;;
        --test-probe-config) test_probe_config=${2-}; test_mode=1 ;;
        --test-runner) test_runner=${2-}; test_mode=1 ;;
        *) fail 'unexpected argument' ;;
    esac
    [ "$#" -ge 2 ] || fail 'test option needs a value'
    shift 2
done

if [ "$test_mode" -eq 1 ]; then
    [ -n "$test_root" ] && [ -n "$test_config" ] && [ -n "$test_ledger" ] &&
        [ -n "$test_dat" ] && [ -n "$test_probe_config" ] && [ -n "$test_runner" ] ||
        fail 'temporary test configuration is incomplete'
    case "$test_root" in /*|[A-Za-z]:/*) ;; *) fail 'temporary root must be absolute' ;; esac
    test_root=$(schway__library_absolute_directory "$test_root") || fail 'temporary root must be absolute'
    for path in "$test_config" "$test_ledger" "$test_dat" "$test_probe_config"; do
        case "$path" in /*|[A-Za-z]:/*) ;; *) fail 'temporary inputs must stay inside the test root' ;; esac
        case "/$path/" in */../*) fail 'temporary input path is invalid' ;; esac
    done
    test_config=$(schway__library_absolute_file "$test_config") || fail 'temporary input path is invalid'
    test_ledger=$(schway__library_absolute_file "$test_ledger") || fail 'temporary input path is invalid'
    test_dat=$(schway__library_absolute_file "$test_dat") || fail 'temporary input path is invalid'
    test_probe_config=$(schway__library_absolute_file "$test_probe_config") || fail 'temporary input path is invalid'
    for path in "$test_config" "$test_ledger" "$test_dat" "$test_probe_config"; do
        case "$path" in "$test_root"/*) ;; *) fail 'temporary inputs must stay inside the test root' ;; esac
        [ ! -L "$path" ] || fail 'temporary input path is invalid'
    done
    case "$test_runner" in /*|[A-Za-z]:/*) ;; *) fail 'test runner is invalid' ;; esac
    case "/$test_runner/" in */../*) fail 'test runner is invalid' ;; esac
    test_runner_directory=$(schway__library_absolute_directory "$(dirname "$test_runner")") || fail 'test runner is invalid'
    test_runner="$test_runner_directory/$(basename "$test_runner")"
    case "$test_runner" in
        "$project_root"/build/*/tests/schwaystation-run-test|"$project_root"/build/*/tests/schwaystation-run-test.exe|"$project_root"/build/*/tests/compat/schwaystation-run-test|"$project_root"/build/*/tests/compat/schwaystation-run-test.exe) ;;
        *) fail 'test runner is invalid' ;;
    esac
    [ -x "$test_runner" ] || fail 'test runner is unavailable'
    [ -f "$test_config" ] && [ -f "$test_ledger" ] && [ -f "$test_dat" ] &&
        [ -f "$test_probe_config" ] && [ -d "$test_root" ] || fail 'temporary input is missing'
    config=$test_config
    disc_dir= bios_dir=
    ledger=$test_ledger
    dat=$test_dat
    probe_config=$test_probe_config
    runner=$test_runner
    output_root=$test_root
else
    [ "$#" -eq 0 ] || fail 'usage: library.sh'
    config=$project_root/.env.local
    [ -f "$config" ] || fail 'copy .env.example to .env.local and set both directories'
    disc_dir= bios_dir= probe_config=
    runner=$project_root/build/library/schwaystation-run
    if [ ! -x "$runner" ] && [ -x "$runner.exe" ]; then runner=$runner.exe; fi
    ledger=$project_root/tests/compat/ledger.csv
    dat="$project_root/build/library/Sony - PlayStation.dat"
    output_root=$project_root
fi

disc_seen=0 bios_seen=0
cr=$(printf '\r')
while IFS= read -r line || [ -n "$line" ]; do
    case "$line" in *"$cr") line=${line%"$cr"} ;; esac
    case "$line" in ''|'#'*) continue ;; *=*) key=${line%%=*}; value=${line#*=} ;; *) fail 'configuration line is malformed' ;; esac
    case "$key" in
        SCHWAYSTATION_DISC_DIR) [ "$disc_seen" -eq 0 ] || fail 'disc directory is repeated'; disc_dir=$value; disc_seen=1 ;;
        SCHWAYSTATION_BIOS_DIR) [ "$bios_seen" -eq 0 ] || fail 'BIOS directory is repeated'; bios_dir=$value; bios_seen=1 ;;
        *) fail 'configuration contains an unsupported key' ;;
    esac
done < "$config"
[ "$disc_seen" -eq 1 ] && [ "$bios_seen" -eq 1 ] && [ -n "$disc_dir" ] && [ -n "$bios_dir" ] ||
    fail 'configuration must set both directories'
disc_dir=$(CDPATH= cd "$disc_dir" 2>/dev/null && pwd -P) || fail 'configured directory is unavailable'
bios_dir=$(CDPATH= cd "$bios_dir" 2>/dev/null && pwd -P) || fail 'configured directory is unavailable'
[ -d "$disc_dir" ] && [ -r "$disc_dir" ] && [ -d "$bios_dir" ] || fail 'configured directory is unavailable'
umask 077
temporary=${TMPDIR:-/tmp}/schway-library-$$
mkdir "$temporary" || fail 'cannot create temporary report'
trap 'rm -rf "$temporary"' 0 HUP INT TERM
if [ "$test_mode" -eq 1 ]; then
    sh "$project_root/scripts/compat-ratchet.sh" "$ledger" "$ledger" "" \
        "$project_root/tests/compat/allow.txt" "$(date -u +%F)" >/dev/null 2>&1 || fail 'temporary ledger is invalid'
else
    cmake --workflow --preset library >/dev/null || fail 'local library workflow failed'
    dat_sha256=52978215854d307bfb5353052c9ce3c34995310a970d586f0100bb67f7de01a8
    if [ ! -f "$dat" ]; then
        curl -fsSL 'https://raw.githubusercontent.com/libretro/libretro-database/12c21cf79f1e20eff6ac68b80875816c717d0a7a/metadat/redump/Sony%20-%20PlayStation.dat' -o "$temporary/library.dat" || fail 'pinned PlayStation DAT download failed'
        [ "$(shasum -a 256 "$temporary/library.dat" | awk '{print $1}')" = "$dat_sha256" ] || fail 'pinned PlayStation DAT digest is invalid'
        cp "$temporary/library.dat" "$dat" || fail 'cannot install pinned PlayStation DAT'
    fi
    [ -f "$ledger" ] || fail 'compatibility ledger is unavailable'
fi

rows=$temporary/rows
cues=$temporary/cue-files
: > "$rows"
: > "$cues"
for cue in "$disc_dir"/*.cue; do
    [ -f "$cue" ] || continue
    awk '
        toupper($1) == "FILE" {
            line=$0
            sub(/^[[:space:]]*[^[:space:]]+[[:space:]]+/, "", line)
            sub(/^[[:space:]]*/, "", line)
            if (substr(line, 1, 1) == "\"") {
                sub(/^\"/, "", line)
                sub(/\".*/, "", line)
            } else {
                sub(/[[:space:]].*$/, "", line)
            }
            if (line != "") print line
        }
    ' "$cue" >> "$cues" 2>/dev/null || fail 'cannot inspect CUE companion metadata'
done
unmatched=0
unsupported=0
report_date=$(date -u +%F)
for image in "$disc_dir"/*.cue "$disc_dir"/*.iso "$disc_dir"/*.bin; do
    [ -f "$image" ] || continue
    case "$image" in
        *.bin)
            name=${image##*/}
            if awk -v name="$name" 'tolower($0) == tolower(name) { found=1 } END { exit !found }' "$cues"; then
                continue
            fi
            ;;
    esac
    if [ "$test_mode" -eq 1 ]; then
        probe=$("$runner" --probe "$image" --test-ledger "$ledger" --test-dat "$dat" \
            --test-root "$test_root" --test-config "$probe_config" 2>/dev/null) || fail 'disc probe refused an input'
    else
        probe=$("$runner" --probe "$image" --metadata-ledger "$ledger" --metadata-dat "$dat" 2>/dev/null) || fail 'disc probe refused an input'
    fi
    serial=$(printf '%s\n' "$probe" | awk -F= '$1 == "serial" { print substr($0, index($0, "=") + 1) }')
    md5=$(printf '%s\n' "$probe" | awk -F= '$1 == "rcheevos_md5" { print substr($0, index($0, "=") + 1) }')
    sha1=$(printf '%s\n' "$probe" | awk -F= '$1 == "track1_sha1" { print substr($0, index($0, "=") + 1) }')
    title=$(printf '%s\n' "$probe" | awk -F= '$1 == "title" { print substr($0, index($0, "=") + 1) }')
    identity=$(printf '%s\n' "$probe" | awk -F= '$1 == "metadata.identity" { print $2 }')
    if [ "$identity" = pass ] &&
        [ -n "$serial" ] && [ -n "$md5" ] && [ -n "$sha1" ] && [ -n "$title" ]; then
        awk -F, -v s="$serial" -v m="$md5" -v h="$sha1" -v t="$title" \
            '$1 == s && $2 == m && $3 == h && $4 == t && $6 == "cue/bin" && $7 == "metadata.identity" && $8 == "pass" { print; n++ } END { if (n != 1) exit 1 }' \
            "$ledger" >> "$rows" || fail 'identity ledger join is ambiguous'
        if [ "$test_mode" -eq 1 ]; then
            for predicate in boots@600 plays@600; do
                awk -F, -v s="$serial" -v m="$md5" -v h="$sha1" -v t="$title" -v p="$predicate" \
                    '$1 == s && $2 == m && $3 == h && $4 == t && $5 == "ntsc-u" && $6 == "cue/bin" && $7 == p && $8 == "unknown" && $9 == "unavailable" { print; n++ } END { if (n != 1) exit 1 }' \
                    "$ledger" >> "$rows" || fail 'unavailable boot/play status is not explicit'
            done
        else
            for predicate in boots@600 plays@600; do
                awk -F, -v s="$serial" -v m="$md5" -v h="$sha1" -v t="$title" -v p="$predicate" -v d="$report_date" \
                    '$1 == s && $2 == m && $3 == h && $4 == t && $6 == "cue/bin" && $7 == "metadata.identity" && $8 == "pass" { print $1 "," $2 "," $3 "," $4 "," $5 "," $6 "," p ",unknown,unavailable:" p "," d; n++ } END { if (n != 1) exit 1 }' \
                    "$ledger" >> "$rows" || fail 'cannot record unavailable boot/play status'
            done
        fi
    else
        unmatched=$((unmatched + 1))
        if [ "$test_mode" -eq 0 ]; then
            for predicate in metadata.identity boots@600 plays@600; do
                printf 'unlisted,unavailable,unavailable,unavailable,unknown,cue/bin,%s,unknown,unavailable,%s\n' \
                    "$predicate" "$report_date" >> "$rows"
            done
        fi
    fi
done
for image in "$disc_dir"/*.chd "$disc_dir"/*.pbp; do
    [ -f "$image" ] && unsupported=$((unsupported + 1))
done

output_dir=$output_root/build/library
mkdir -p "$output_dir"
printf '%s\n' 'serial,md5,track1_sha1,title,region,media,predicate,status,evidence,date' > "$temporary/results.csv"
LC_ALL=C sort -u "$rows" >> "$temporary/results.csv"
mv -f "$temporary/results.csv" "$output_dir/results.csv"
printf '%s\n' "library: unmatched=$unmatched unsupported=$unsupported"
