PATH="_CURVEPROTECT_/bin:${PATH}"
export PATH

if [ ! $# -eq 2 ]; then
  echo "forwarder-conf: usage: forwarder-conf /directory /keydirectory"
  exit 100
fi

dir=$1
keydir=$2


mkdir -p "${dir}/log/main"
mkdir -p "${dir}/env"
echo "${dir}/root" > "${dir}/env/ROOT"
echo "${keydir}" > "${dir}/env/KEYDIR"

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
  echo "  if [ x\"\${REMOTEHOST}\" = x ]; then echo \"\\\$REMOTEHOST not set \"; exit 111; fi"
  echo "  if [ x\"\${REMOTEPORT}\" = x ]; then echo \"\\\$REMOTEPORT not set \"; exit 111; fi"
  echo "  if [ x\"\${KEYDIR}\" = x ]; then echo \"\\\$KEYDIR not set \"; exit 111; fi"
  echo "  echo \"starting server forwarder\""
  echo "  exec netcurvecpserver \"\${NAME}\" \"\${KEYDIR}\" \"\${IP}\" \"\${PORT}\" \"\${EXTENSION}\" netcurvecpmessage netclient -u \"\${REMOTEHOST}\" \"\${REMOTEPORT}\" fdcopy ................................................................"
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
