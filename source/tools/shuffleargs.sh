#20121218
#Jan Mojzis
#Public domain.

#zero arguments
shuffleargs

#shuffleargs
(
  for i in `awk 'BEGIN{for(i=0;i<100;++i)print i;exit}'`; do
    x=`shuffleargs a b`;
    echo ${x}; 
  done
) | sort -u | wc -l |\
while read line; do
  if [ x"${line}" != x2 ]; then
    echo "shuffleargs: test failed"; exit 111
  fi
done

#disk full
if [ -w /dev/full ]; then
  e=`shuffleargs a b 2>&1 > /dev/full|| :`
  if [ x"${e}" != x"shuffleargs: fatal: unable to write output: out of disk space" ]; then
    echo "${e}"; exit 111
  fi
fi
