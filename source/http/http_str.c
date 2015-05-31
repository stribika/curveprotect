#include "http.h"

#define X(e,s) if (i == e) return s;


const char *http_str(int i){
  X(200,"OK")
  X(201,"Created")
  X(202,"Accepted")
  X(203,"Non-Authoritative Information")
  X(204,"No Content")
  X(205,"Reset Content")
  X(206,"Partial Content")
  X(207,"Multi-Status")
  X(300,"Multiple Choices")
  X(301,"Moved Permanently")
  X(302,"Found")
  X(303,"See Other")
  X(304,"Not Modified")
  X(305,"Use Proxy")
  X(306,"unused")
  X(307,"Temporary Redirect")
  X(400,"Bad Request")
  X(401,"Authorization Required")
  X(402,"Payment Required")
  X(403,"Forbidden")
  X(404,"Not Found")
  X(405,"Method Not Allowed")
  X(406,"Not Acceptable")
  X(407,"Proxy Authentication Required")
  X(408,"Request Time-out")
  X(409,"Conflict")
  X(410,"Gone")
  X(411,"Length Required")
  X(412,"Precondition Failed")
  X(413,"Request Entity Too Large")
  X(414,"Request-URI Too Large")
  X(415,"Unsupported Media Type")
  X(416,"Requested Range Not Satisfiable")
  X(417,"Expectation Failed")
  X(418,"unused")
  X(419,"unused")
  X(420,"unused")
  X(421,"unused")
  X(422,"Unprocessable Entity")
  X(423,"Locked")
  X(424,"Failed Dependency")
  X(425,"No code")
  X(426,"Upgrade Required")
  X(500,"Internal Server Error")
  X(501,"Method Not Implemented")
  X(502,"Bad Gateway")
  X(503,"Service Temporarily Unavailable")
  X(504,"Gateway Time-out")
  X(505,"HTTP Version Not Supported")
  X(506,"Variant Also Negotiates")
  X(507,"Insufficient Storage")
  X(508,"unused")
  X(509,"unused")
  X(510,"Not Extended")
  return "unused";
}