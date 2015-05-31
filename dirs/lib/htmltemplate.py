# 20111025
# Jan Mojzis
# Public domain.

import sys

class TemplateError(Exception):
        pass

class Template:
        """
        """


        def __init__(self, fn = ""):
                """
                """

                self.template = ""
                self.template = self._rread(fn, self.template)

        def _rread(self, fn, data):
                """
                """

                f = open(fn)
                while True:
                        line = f.readline()
                        if not len(line):
                                break
                        if len(line) and line[-1] == "\n":
                                line = line[0:-1]
                        if len(line) and line[-1] == "\r":
                                line = line[0:-1]

                        if line[0:22] == "%TEMPLATE_INCLUDE_FILE":
                                (dummy, fn2) = line.split(" ", 1)
                                fn2 = fn2.strip()
                                data = self._rread(fn2, data)
                        else:
                                data = "%s%s\n" % (data,line)
                f.close()
                return data

        def _replace(self, d, b):
                """
                """

                for key in d:
                        if type(d[key]) != type("str"):
                                continue
                        s = "%%TEMPLATE_VARIABLE_%s%%" % (key)
                        b = b.replace(s,d[key])

                return b

        def _catch_blockbegin(self, line):
                """
                """

                mask = "%TEMPLATE_BLOCKBEGIN_"
                maskl = len(mask)

                if line.find(mask) == -1:
                        return None

                try:
                        x = line.split("%")
                        if len(x) != 3:
                                raise Exception("x")
                        return x[1][20:]
                except:
                        raise TemplateError("Malformed template line: %s" % (line))

        def _catch_blockend(self, line, name):
                """
                """

                mask = "%%TEMPLATE_BLOCKEND_%s%%" % (name) 
                if line.strip().find(mask) == -1:
                        return False
                return True


        def _rblock(self, tdict, template):
                """
                """

                block  = ""
                flagblock = 0
                ret = ""
                name = ""

                for line in template.split("\n"):

                        #XXX
                        if not len(line):
                                continue

                        if not flagblock:
                                name = self._catch_blockbegin(line)
                                if name:
                                        values = tdict[name.lower()]
                                        flagblock = 1
                                        continue

                        if flagblock:
                                if self._catch_blockend(line, name):
                                        flagblock = 0
                                        for value in values:
                                                #ret += self._rblock(value, block)
                                                ret = "%s%s" %  (ret,self._rblock(value, block))
                                        block = ""
                                        continue

                        if flagblock:
                                block = "%s%s\n" % (block,line)
                        else:
                                ret = "%s%s\n" % (ret,line)
                ret = self._replace(tdict, ret)
                r = ret.find("%TEMPLATE")
                if r != -1:
                        xxx = ret[r:].split('%')[1]
                        raise TemplateError("Uncatched template variable: %s" % (xxx))

                return ret

        def write(self, tdict):
                """
                """

                ret = ""
                ret = self._rblock(tdict, self.template)
                r = ret.find("%TEMPLATE")
                if r != -1:
                        xxx = ret[r:].split('%')[1]
                        raise TemplateError("Uncatched template variable: %s" % (xxx))
                #print ret
                sys.stdout.write(ret)
                sys.stdout.flush()

