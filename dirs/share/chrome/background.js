// 20120725
// Jan Mojzis
// Public domain.

var debug=false;
var globals = new Array();

chrome.webRequest.onCompleted.addListener (

  // details
  function(details) {
    var tabId = details.tabId;
    var proxyIp = localStorage["curveprotectProxyIp"] || defaultProxyIp;
    var proxyPort = localStorage["curveprotectProxyPort"] || defaultProxyPort;
    var proxyHeader = localStorage["curveprotectProxyHeader"] || defaultProxyHeader;


    if (details.type == "main_frame") {
      if (globals[tabId] == undefined) globals[tabId] = new Object();
      globals[tabId].url = details.url;
      globals[tabId].icon = "icon-red.png";
      if (proxyIp != details.ip) {
        globals[tabId].icon = "icon-black.png";
        return;
      }
    }
    else {
      //TODO
      return;
    }

    for(i = 0; i < details.responseHeaders.length; ++i) {
      if (details.responseHeaders[i].name.toLowerCase() == proxyHeader.toLowerCase()) {
        if (debug) console.log("protected: " + details.tabId + " " +  details.url);
        globals[tabId].icon = "icon-green.png";
      }
    }
  },

  // filters
  {
    urls: [
      "http://*/*",
    ]
  },

  //extradetailsSpec
  //['responseHeaders', 'blocking']
  ['responseHeaders']
);

chrome.tabs.onUpdated.addListener (
  function(tabId, a, b) {
    if (debug) console.log("chrome.tabs.onUpdated: " + JSON.stringify(tabId) + " " + JSON.stringify(a));
    if (tabId < 0) return;
    if (globals[tabId] == undefined) return;
    if (globals[tabId].icon == undefined) return;

    if (a.status == "loading") {
      if (a.url == undefined) return; //nothing changed
      if (a.url == globals[tabId].url) return; //nothing changed
      globals[tabId].icon = "icon-black.png";
      return;
    }

    if (a.status == "complete") {
      chrome.browserAction.setIcon({path:globals[tabId].icon, tabId:tabId});
      return;
    }
  }
);

chrome.tabs.onCreated.addListener (
  function(details) {
    var tabId = details.id;
    if (debug) console.log("chrome.tabs.onCreated: " + " " +  JSON.stringify(details));
    if (tabId < 0) return;
    if (globals[tabId] == undefined) return;
    if (globals[tabId].icon == undefined) return;
    chrome.browserAction.setIcon({path:globals[tabId].icon, tabId:tabId});
  }
)

chrome.tabs.onRemoved.addListener (
  function(tabId, details) {
    if (debug) console.log("chrome.tabs.onRemoved: " + " " + tabId + " " + JSON.stringify(details));
    if (tabId < 0) return;
    if (globals[tabId] == undefined) return;
    globals[tabId] = null;
  }
)
