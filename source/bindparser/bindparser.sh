#201301221
#Jan Mojzis
#Public domain.

LANG=C
export LANG
LC_ALL=C
export LC_ALL

for zone in root-servers.net in-addr.arpa root arpa lala 2.10.in-addr.arpa; do
  ORIGIN="${zone}."
  if [ x"${zone}" = xroot ]; then
    ORIGIN="."
  fi
  export ORIGIN
  ./bindparser "bind-${zone}" | sort > "out-${zone}"
  cmp "exp-${zone}" "out-${zone}" || { echo "bad result for zone ${zone}"; exit 111; }
done
