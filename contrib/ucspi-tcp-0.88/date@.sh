HOME/bin/tcpclient -RHl0 -- "${1-0}" 13 sh -c 'exec HOME/bin/delcr <&6' | cat -v
