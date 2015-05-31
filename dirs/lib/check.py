# 20111025
# Jan Mojzis
# Public domain.

import socket

def zone(name = ""):


        if name.find("/") != -1:
                raise ValueError("Sorry, name can't contain string '/'")

        if name.find("..") != -1:
                raise ValueError("Sorry, name can't contain '..'")

        if len(name) > 1 and name[0] == ".":
                raise ValueError("Sorry, first character can't be '.'")

        return True

allowed="0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-"
hexdigits="0123456789abcdef"

def name(name = ""):
        for n in name:
                if allowed.find(n) == -1:
                        raise ValueError("Sorry, only '%s' characters allowed" % (allowed))

def ip(s = ""):

        s = s.split('\n')[0]

        try:
                socket.inet_aton(s)
        except:
                try:
                        x = socket.AF_INET6
                except:
                        return
                else:
                        socket.inet_pton(socket.AF_INET6, s)
        else:
                return

def ip4(s = ""):
        """
        """

        s = s.split('\n')[0]
        a = s.split(".")
        if len(a) != 4:
                raise ValueError("malformed IP")

        for i in [0,1,2,3]:
                if int(a[i]) > 255 or  int(a[i]) < 0:
                        raise ValueError("malformed IP")
        return

def hexkey(s = ""):
        """
        """

        #not compatible with python3
        for k in s.lower():
                if hexdigits.find(k) == -1:
                        raise ValueError("Sorry, only '%s' characters allowed" % (hexdigits))

def port(port = 0):
        """
        """

        if port < 1:
                raise ValueError("port must be in range 1 - 65535")
        if port > 65535:
                raise ValueError("port must be in range 1 - 65535")
        return


