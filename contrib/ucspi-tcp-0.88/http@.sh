echo "GET /${2-} HTTP/1.0
Host: ${1-0}:${3-80}
" | HOME/bin/tcpclient -RHl0 -- "${1-0}" "${3-80}" sh -c '
  HOME/bin/addcr >&7
  exec HOME/bin/delcr <&6
' | awk '/^$/ { body=1; next } { if (body) print }'
