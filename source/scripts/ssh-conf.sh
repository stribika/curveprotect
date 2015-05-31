#!/bin/sh -e

PATH="_CURVEPROTECT_/bin:${PATH}"
export PATH

if [ ! $# -eq 2 ]; then
  echo "ssh-conf: usage: ssh-conf /directory /keydirectory"
  exit 100
fi

dir=$1
keydir=$2

mkdir -p "${dir}/log/main"
mkdir -p "${dir}/env"
echo "${keydir}" > "${dir}/env/KEYDIR"

(
  echo "#!/bin/sh"
  echo "exec 2>&1"
  echo "PATH=\"_CURVEPROTECT_/bin:\${PATH}\""
  echo "export PATH"
  echo "exec envdir ./env sh -ec '"
  echo "  if [ x\"\${EXTENSION}\" = x ]; then echo \"\\\$EXTENSION not set \"; exit 111; fi"
  echo "  if [ x\"\${IP}\" = x ]; then echo \"\\\$IP not set \"; exit 111; fi"
  echo "  if [ x\"\${PORT}\" = x ]; then echo \"\\\$PORT not set \"; exit 111; fi"
  echo "  if [ x\"\${KEYDIR}\" = x ]; then echo \"\\\$KEYDIR not set \"; exit 111; fi"
  echo "  if [ x\"\${NAME}\" = x ]; then echo \"\\\$NAME not set \"; exit 111; fi"
  echo "  echo \"starting SSHCurve server\""
  echo "  exec netcurvecpserver \"\${NAME}\" \"\${KEYDIR}\" \"\${IP}\" \"\${PORT}\" \"\${EXTENSION}\" netcurvecpmessage sshd -i -e -D"
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
