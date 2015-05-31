#!/bin/sh -e

PATH="_CURVEPROTECT_/bin:${PATH}"
export PATH

if [ ! $# -eq 3 ]; then
  echo "vpn-conf: usage: vpn-conf acct /directory /keydirectory"
  exit 100
fi

usr=$1
dir=$2
keydir=$3

mkdir -p "${dir}/log/main"
mkdir -p "${dir}/env"
mkdir -p "${dir}/root/lock"
echo "${dir}/root" > "${dir}/env/ROOT"
echo "${keydir}" > "${dir}/env/KEYDIR"

echo 'data.cdb: data
	_CURVEPROTECT_/bin/vpn-data
' > "${dir}/root/Makefile"

echo "
# -- EXAMPLE ------------------------------------------------------------------------------------------------------------------
# #defaut routes
# r::192.168.0.0:255.255.0.0
# r::10.2.0.0:255.255.0.0
# r::172.16.85.0:255.255.255.0
# 
# #IP address for the first user with public key 84fb6a942b1bcddff2cfd231d012d5d586b710ff180b6841da721eea5ee4004e
# i:84fb6a942b1bcddff2cfd231d012d5d586b710ff180b6841da721eea5ee4004e:172.16.85.3:172.16.85.4
# 
# #IP address for the second user with public key 68585b65041498f7b9f300ef890169cbc4b55927e9d15f9b3651109f338bf35e + extra route
# i:68585b65041498f7b9f300ef890169cbc4b55927e9d15f9b3651109f338bf35e:172.16.85.5:172.16.85.6
# r:68585b65041498f7b9f300ef890169cbc4b55927e9d15f9b3651109f338bf35e:10.0.0.0:255.255.0.0
# -- EXAMPLE ------------------------------------------------------------------------------------------------------------------
" > "${dir}/root/data"

(
  echo "#!/bin/sh -e"
  echo "exec 2>&1"
  echo "PATH=\"_CURVEPROTECT_/bin:\${PATH}\""
  echo "export PATH"
  echo "exec envdir ./env sh -ec '"
  echo "  if [ x\"\${NAME}\" = x ]; then echo \"\\\$NAME not set \"; exit 111; fi"
  echo "  if [ x\"\${EXTENSION}\" = x ]; then echo \"\\\$EXTENSION not set \"; exit 111; fi"
  echo "  if [ x\"\${IP}\" = x ]; then echo \"\\\$IP not set \"; exit 111; fi"
  echo "  if [ x\"\${PORT}\" = x ]; then echo \"\\\$PORT not set \"; exit 111; fi"
  echo "  if [ x\"\${ROOT}\" = x ]; then echo \"\\\$ROOT not set \"; exit 111; fi"
  echo "  if [ x\"\${KEYDIR}\" = x ]; then echo \"\\\$KEYDIR not set \"; exit 111; fi"
  echo "  ("
  echo "    cd \"\${ROOT}/lock\""
  echo "    rm -f ????????????????????????????????????????????????????????????????"
  echo "  )"
  echo "  echo \"starting VPN server\""
  echo "  cd \"\${ROOT}\"" #XXX
  echo "  exec envuidgid ${usr} netcurvecpserver \"\${NAME}\" \"\${KEYDIR}\" \"\${IP}\" \"\${PORT}\" \"\${EXTENSION}\" netcurvecpmessage vpn -s ................................................................"
  echo "'"
) > "${dir}/run.tmp"
chmod 755 "${dir}/run.tmp"
mv -f "${dir}/run.tmp" "${dir}/run"

(
  echo "#!/bin/sh"
  echo "PATH=\"_CURVEPROTECT_/bin:\${PATH}\""
  echo "export PATH"
  echo ""
  echo ""
  echo "exec extremeenvuidgid sh -c '"
  echo "  uidgidchown -R ./main"
  echo "  exec extremesetuidgid multilog t ./main"
  echo "'"
) > "${dir}/log/run.tmp"
chmod 755 "${dir}/log/run.tmp"
mv -f "${dir}/log/run.tmp" "${dir}/log/run"

