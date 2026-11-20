#ifndef Haitham_HTTPREQUEST_H
#define Haitham_HTTPREQUEST_H
#include "../../DataStructures/Dictionary/Dictionary.h"
struct HTTPRequest { struct Dictionary request_line,header_fields,body; };
struct HTTPRequest http_request_constructor(char *request_string);
void http_request_destructor(struct HTTPRequest *request);
#endif
