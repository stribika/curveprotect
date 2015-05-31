#!/bin/sh

if [ x"`dirname $0`" != x_CURVEPROTECT_/sbin ]; then
  echo "$0: unable to run script from _CURVEPROTECT_/bin directory" >&2
  exit 111
fi


#systemd
if [ -d /lib/systemd/system ]; then
  if systemctl show-environment 1>/dev/null 2>/dev/null; then
    service="curveprotect.service"
    if systemctl is-active "${service}" 1>/dev/null 2>/dev/null; then
      systemctl stop "${service}"
    fi
    if systemctl is-enabled "${service}" 1>/dev/null 2>/dev/null; then
      systemctl disable "${service}"
    fi
    if [ -f "/lib/systemd/system/${service}" ]; then
      rm -f "/lib/systemd/system/${service}"
    fi
    exit 0
  fi
fi

#upstart
if [ -d /etc/init ]; then
  if initctl list 1>/dev/null 2>/dev/null; then
    job="curveprotect"
    if [ -f "/etc/init/${job}.conf" ]; then
      initctl stop "${job}"
      rm -f "/etc/init/${job}.conf"
    fi
    exit 0
  fi
fi

#macosx 
if [ -d /System/Library/LaunchDaemons ]; then
  if launchctl managerpid 1>/dev/null 2>/dev/null; then
    plist=com.mojzis.curveprotect.plist
    if [ -f "/System/Library/LaunchDaemons/${plist}" ]; then
      (
        cd /System/Library/LaunchDaemons/
        launchctl unload "${plist}"
        rm -f "${plist}"
      )
    fi
  fi
  exit 0
fi

#inittab
if [ -r /etc/inittab ]; then
  if grep _CURVEPROTECT_ /etc/inittab >/dev/null; then
    _CURVEPROTECT_/bin/setlock /etc/inittab sh -c "
      rm -f /etc/inittab'{new}'
      grep -v _CURVEPROTECT_ /etc/inittab > /etc/inittab'{new}'
      mv -f /etc/inittab'{new}' /etc/inittab
      kill -HUP 1
    "
  fi
  exit 0
fi

#rc.local
if [ -r /etc/rc.local ]; then
  if grep _CURVEPROTECT_ /etc/rc.local >/dev/null; then
    _CURVEPROTECT_/bin/setlock /etc/rc.local sh -c "
      rm -f /etc/rc.local'{new}'
      grep -v _CURVEPROTECT_ /etc/rc.local > /etc/rc.local'{new}'
      mv -f /etc/rc.local'{new}' /etc/rc.local
    "
  fi
  exit 0
fi
