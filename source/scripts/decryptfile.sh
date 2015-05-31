#!/bin/sh

PATH="_CURVEPROTECT_/bin:${PATH}"
export PATH

if [ "$#" -ne 3 ]; then
  echo "decryptfile: usage: decryptfile srcfn dstfn tmpfn"
  exit 100
fi

if [ ! -f "$1" ]; then
  echo "decryptfile: fatal: unable to open file $1: file does not exist"
  exit 111
fi

echo "type password:"
read PASSWORD; export PASSWORD

decrypt <"$1" >"$3" || exit 111
mv -f "$3" "$2" || exit 111
exit 0
