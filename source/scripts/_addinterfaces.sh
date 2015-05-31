#!/bin/sh 

PATH="_CURVEPROTECT_/bin:${PATH}"
export PATH

IPS="_CURVEPROTECTIPS_"

#ip addr
if ip addr show lo 1>/dev/null 2>/dev/null; then
  #linux
  for ip in ${IPS}; do 
    ip addr del "${ip}/8" dev lo 1>/dev/null 2>/dev/null || :
    ip addr add "${ip}/8" dev lo
  done
  exit 0
fi

#solaris ifconfig
if killafter 1 ifconfig lo0 plumb 1>/dev/null 2>/dev/null; then
  #solaris
  for ip in ${IPS}; do
    id=`echo "${ip}" | awk 'BEGIN{FS="."}{print $4}'`
    ifconfig "lo0:${id}" plumb "${ip}" netmask 255.0.0.0 up
  done
  exit 0
fi

#BSD ifconfig
if ifconfig lo0 1>/dev/null 2>/dev/null; then
  #macosx, aix, freebsd, openbsd, netbsd
  for ip in ${IPS}; do
    #ifconfig lo0 alias "${ip}"
    #ifconfig lo0 inet "${ip}" alias
    ifconfig lo0 "${ip}" netmask 255.0.0.0 alias
  done
  exit 0
fi
