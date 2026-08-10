#ifndef COVA_MULTIPART_H
#define COVA_MULTIPART_H

#include "request.h"

// Parse multipart/form-data body and populate req->files
void multipart_parse(Request *req);

#endif
