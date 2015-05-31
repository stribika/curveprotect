#!/bin/sh

PATH="_CURVEPROTECT_/bin:${PATH}"
export PATH

cfg=_CPCFG_
export cfg

service=$1
export service

if [ x"${service}" = x ]; then
  echo "fixservice: usage: fixservice servicename"
  exit 100
fi

envuidgid "${cfg}" sh -c '
  cd "_CURVEPROTECT_/servicedir/${service}"
  trycreatesupervise
  uidgidchown supervise supervise/ok supervise/control
  cd "_CURVEPROTECT_/servicedir/${service}/log"
  trycreatesupervise
  uidgidchown supervise supervise/ok supervise/control

  case "${service}" in \
    vpn*) 
      uidgidchown -R "_CURVEPROTECT_/etc/${service}/root"
      uidgidchown -R "_CURVEPROTECT_/etc/${service}/env"
    ;;
    dnscache) 
      uidgidchown -R "_CURVEPROTECT_/etc/dnscache/root/servers"
      uidgidchown -R "_CURVEPROTECT_/etc/${service}/env"
    ;;
    dnslocal) 
      uidgidchown -R "_CURVEPROTECT_/etc/dnslocal/conf"
      uidgidchown -R "_CURVEPROTECT_/etc/${service}/env"
    ;;
    addremoveservice) 
    ;;
    dnslocaldownload) 
    ;;
    dnscryptcachedownload) 
    ;;
    dnslocaltcp) 
    ;;
    *) 
      uidgidchown -R "_CURVEPROTECT_/etc/${service}/env"
    ;;
  esac
'
