#!/bin/sh
PATH="_CURVEPROTECT_/bin:${PATH}"
export PATH
exec netclient ${1+"$@"} fdcopy
