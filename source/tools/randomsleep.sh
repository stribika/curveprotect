#!/bin/sh -e

#20130103
#Jan Mojzis
#Public domain.

PATH="`pwd`:${PATH}"; export PATH

#help
if [ x"`randomsleep -h 2>&1 | grep '^randomsleep: usage:'`" != x"randomsleep: usage:" ]; then
  echo "randomsleep: help error"; exit 111
fi

#bad/negative input
for i in a '' -1 -01 100000000000000000000000000000000000000; do
  e=`randomsleep "${i}" 2>&1 | tail -1`
  if [ x"${e}" != x"randomsleep: fatal: unable to parse number1" ]; then
    echo "${e}"; exit 111
  fi
done

#bad/negative input
for i in a -1 -01 100000000000000000000000000000000000000; do
  e=`randomsleep 1 "${i}" 2>&1 | tail -1`
  if [ x"${e}" != x"randomsleep: fatal: unable to parse number2" ]; then
    echo "${e}"; exit 111
  fi
done

#number2 < number1
e=`randomsleep 1 0 2>&1 | tail -1`
if [ x"${e}" != x"randomsleep: fatal: number2 must be greater or equal than number1" ]; then
  echo "${e}"; exit 111
fi

#zero length
for i in 0 +0 -0 000; do
  randomsleep "${i}"
done



