# 20110802
# Jan Mojzis
# Public domain.

import sys, os, config


def header(x = "", page = "", seconds = 0):

        ret = {}
        ret["name"] = x
        ret["redirect"] = []

        if page:
                tmp = {}
                tmp["seconds"] = str(seconds)
                tmp["page"] = page
                ret["redirect"].append(tmp)

        ret["indexclass"]=""
        ret["dns_statsclass"] =""
        ret["dnsclass"] = ""
        ret["dnscryptclass"] = ""
        ret["forwarderclass"] = ""
        ret["httpproxy_statsclass"] =""
        ret["proxyclass"] = ""
        ret["vpnclass"] = ""
        ret["dnscurvekeysclass"] = ""
        ret["curvecpkeysclass"] = ""
        ret["ed25519keysclass"] = ""
        ret["gpgkeysclass"] = ""
        ret["backupclass"] = ""
        ret["troubleshootingclass"] = ""
        ret["version"] = config.version

        #XXX
        ret["dns_statsclass"] = ""
        ret["httpproxy_statsclass"] = ""

        if x == "index":
                ret["indexclass"]="hi"
        if x == "dns":
                ret["dnsclass"] = "hi"
        if x == "dnscrypt":
                ret["dnscryptclass"] = "hi"
        if x == "dns_stats":
                ret["dns_statsclass"]="hi"
        if x == "forwarder":
                ret["forwarderclass"] = "hi"
        if x == "httpproxy_stats":
                ret["httpproxy_statsclass"] ="hi"
        if x == "proxy":
                ret["proxyclass"] = "hi"
        if x == "vpn":
                ret["vpnclass"] = "hi"
        if x == "dnscurvekeys":
                ret["dnscurvekeysclass"] = "hi"
        if x == "curvecpkeys":
                ret["curvecpkeysclass"] = "hi"
        if x == "backup":
                ret["backupclass"] = "hi"
        if x == "ed25519keys":
                ret["ed25519keysclass"] = "hi"
        if x == "gpgkeys":
                ret["gpgkeysclass"] = "hi"
        if x == "troubleshooting":
                ret["troubleshootingclass"] = "hi"

        ret["flagfailed"] = []

        qs = os.getenv("QUERY_STRING")
        if qs:
                if qs == "failed":
                        ret["flagfailed"]=[{"name":x}]

        return ret

