#!/bin/sh 

#20130119
#Jan Mojzis
#Public domain.

(
  echo 'trap "{ echo $0 test failed; exit 111; }" EXIT TERM'
  echo 'PATH="`pwd`:${PATH}"; export PATH'
  cat "$1.sh"
  echo 'trap "exit 0" EXIT'
) > "rts-$1.sh"
exec sh -e rts-$1.sh
