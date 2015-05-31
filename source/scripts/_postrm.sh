#!/bin/sh

if [ x"`dirname $0`" != x_CURVEPROTECT_/sbin ]; then
  echo "$0: unable to run script from _CURVEPROTECT_/bin directory" >&2
  exit 111
fi

#remove users and groups
usr=_CPUSR_
cfg=_CPCFG_
_CURVEPROTECT_/sbin/_removeuser "${usr}" || :
_CURVEPROTECT_/sbin/_removeuser "${cfg}" || :
_CURVEPROTECT_/sbin/_removegroup "${usr}" || :
_CURVEPROTECT_/sbin/_removegroup "${cfg}" || :

#cleanup
(
  cd _CURVEPROTECT_ && \
  (
    rm -rf bin etc html lib log servicedir service share var www tmp sbin
  )
)
