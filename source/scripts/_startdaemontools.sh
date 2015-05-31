#!/bin/sh

if [ x"`dirname $0`" != x_CURVEPROTECT_/sbin ]; then
  echo "$0: unable to run script from _CURVEPROTECT_/bin directory" >&2
  exit 111
fi

#systemd
if [ -d /lib/systemd/system ]; then
  if systemctl show-environment 1>/dev/null 2>/dev/null; then
    service="curveprotect.service"
    if systemctl is-enabled "${service}" 1>/dev/null 2>/dev/null; then
      echo "Service ${service} is enabled. I assume that curveprotect is already running."
    else
      (
        echo '[Unit]'
        echo 'Description=curveprotect'
        echo ''
        echo '[Service]'
        echo 'ExecStart=_CURVEPROTECT_/bin/svscanboot'
        echo 'Restart=always'
        echo 'StandardOutput=null'
        echo ''
        echo '[Install]'
        echo 'WantedBy=multi-user.target'
        echo 'Alias=curveprotect.target'
      ) > "/lib/systemd/system/${service}.tmp"
      mv -f "/lib/systemd/system/${service}.tmp" "/lib/systemd/system/${service}"
      systemctl enable "${service}"
      systemctl start "${service}"
    fi
    exit 0
  fi
fi

#upstart
if [ -d /etc/init ]; then
  if initctl list 1>/dev/null 2>/dev/null; then
    job="curveprotect"
    if  [ -f "/etc/init/${job}.conf" ]; then
      echo "File /etc/init/${job}.conf exist. I assume that curveprotect is already running."
    else
      (
        echo 'start on runlevel [12345]'
        echo 'stop on runlevel [06]'
        echo 'respawn'
        echo 'exec _CURVEPROTECT_/bin/svscanboot'
      ) > "/etc/init/${job}.conf.tmp"
      mv -f "/etc/init/${job}.conf.tmp" "/etc/init/${job}.conf"
      initctl start "${job}"
    fi
    exit 0
  fi
fi

#macosx 
if [ -d /System/Library/LaunchDaemons ]; then
  if launchctl managerpid 1>/dev/null 2>/dev/null; then
    plist=com.mojzis.curveprotect.plist
    if [ -f "/System/Library/LaunchDaemons/${plist}" ]; then
      echo "LaunchDaemon ${plist} exist. I assume that curveprotect is already running."
    else
      (
        cd /System/Library/LaunchDaemons/
        rm -f "${plist}.tmp"
        (
          echo "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
          echo "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
          echo "<plist version=\"1.0\">"
          echo "<dict>"
          echo "        <key>Label</key>"
          echo "        <string>${plist}</string>"
          echo "        <key>ServiceDescripttion</key>"
          echo "        <string>curveprotect</string>"
          echo "        <key>OnDemand</key>"
          echo "        <false/>"
          echo "        <key>UserName</key>"
          echo "        <string>root</string>"
          echo "        <key>ProgramArguments</key>"
          echo "        <array>"
          echo "                <string>_CURVEPROTECT_/bin/svscanboot</string>"
          echo "        </array>"
          echo "</dict>"
          echo "</plist>"
        ) > "${plist}.tmp"
        mv -f "${plist}.tmp" "${plist}"
        launchctl load "${plist}"
      )
    fi
    exit 0
  fi
fi

#inittab
if [ -r /etc/inittab ]; then
  if grep _CURVEPROTECT_ /etc/inittab >/dev/null; then
    echo 'inittab contains an curveprotect line. I assume that curveprotect is already running.'
  else
    _CURVEPROTECT_/bin/setlock /etc/inittab sh -c "
      rm -f /etc/inittab'{new}'
      ( 
        cat /etc/inittab
        echo 'Cp:12345:respawn:_CURVEPROTECT_/bin/svscanboot' 
      ) > /etc/inittab'{new}'
      mv -f /etc/inittab'{new}' /etc/inittab
      kill -HUP 1
    "
  fi
  exit 0
fi

#fallback to rc.local
if grep _CURVEPROTECT_ /etc/rc.local >/dev/null; then
  echo 'rc.local contains an curveprotect line. I assume that curveprotect is already running.'
else
  _CURVEPROTECT_/bin/setlock /etc/rc.local sh -c "
    rm -f /etc/rc.local'{new}'
    ( 
      cat /etc/rc.local
      echo 'csh -cf \"_CURVEPROTECT_/bin/svscanboot &\"'
    ) > /etc/rc.local'{new}'
    mv -f /etc/rc.local'{new}' /etc/rc.local
  "
  csh -cf '_CURVEPROTECT_/bin/svscanboot &'
fi
exit 0
