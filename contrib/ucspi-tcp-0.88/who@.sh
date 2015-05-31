HOME/bin/tcpclient -RHl0 -- "${1-0}" 11 sh -c 'exec HOME/bin/delcr <&6' | cat -v
