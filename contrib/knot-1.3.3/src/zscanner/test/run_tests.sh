#!/bin/sh

TESTS_DIR="/home/ondrej/Projects/knot/src/zscanner/test/cases"
OUTS_DIR="/home/ondrej/Projects/knot/src/zscanner/test/.out"
TEST_BIN="./../zscanner-tool -m 2"

# Delete temporary output directory at exit.
trap "chmod -R u+rw ${OUTS_DIR} && rm -rf ${OUTS_DIR}" EXIT

# If an argument -> verbose mode (stores result in /tmp).
if [ $# -eq 0 ]; then
	RESULT_DIR=`mktemp -d /tmp/zscanner_test.XXXX`
	echo "ZSCANNER TEST ${RESULT_DIR}"
fi

# Create output directory.
mkdir -p "${OUTS_DIR}"

# Run zscanner on all test zone files.
for file in $(find "${TESTS_DIR}" -name "*.in" | sort -n); do
	fileout="$(basename "${file}" .in).out"

	# Run zscanner.
	${TEST_BIN} . "${file}" > "${OUTS_DIR}/${fileout}"

	# Compare result with the reference one.
	cmp -s "${OUTS_DIR}/${fileout}" "${TESTS_DIR}/${fileout}"

	RET=$?

	# Check for differences.
	if [ $RET -ne 0 ]; then
		# If verbose print diff.
		if [ $# -eq 0 ]; then
			echo "\n=== ${fileout} DIFF ======================"
			diff "${OUTS_DIR}/${fileout}" "${TESTS_DIR}/${fileout}"
		fi
	fi
done

# Store test result.
if [ $# -eq 0 ]; then
	cp -a "${OUTS_DIR}/." "${RESULT_DIR}/"
	echo "\nFINISHED ${RESULT_DIR}"
fi
