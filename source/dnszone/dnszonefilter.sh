#20130627
#Jan Mojzis
#Public domain.

#help TODO
#if [ x"`dnszonefilter -h 2>&1 | grep '^dnszonefilter: usage:'`" != x"dnszonefilter: usage:" ]; then
#  echo "dnszonefilter test failed: help error"; exit 111
#fi

for fn in `ls -1 *.txt`; do
  out=`echo "${fn}" | sed 's/\.txt/.out/'`
  exp=`echo "${fn}" | sed 's/\.txt/.exp/'`
  zone=`echo "${fn}" | sed 's/\.txt//' | cut -d '-' -f2`
  dnszonefilter "${zone}" < "${fn}" > "${out}"
  cmp "${exp}" "${out}" || { echo "dnszonefilter test ${fn}: failed"; exit 111; }
done
