#! /bin/bash
set -x

((FAILED=0))
((SUCCEEDED=0))

# TODO(joe): move this and other generally useful funcs into library.
DbStats() {
    WEB_SERVER_PORT="${WEB_SERVER_PORT-8080}"
    # cbfs dbstats > dbstats.out
    wget "http://localhost:${WEB_SERVER_PORT}/DBSTATS" -O dbstats.out
    cat dbstats.out
}

# Not verifying anything yet - just testing output (for development).
DbStats

#####
echo "Done with $0: $SUCCEEDED succeeded, $FAILED failed"
echo 
if ((FAILED > 0))
then
    exit 1
fi
exit 0
