#ifndef COVA_MIME_H
#define COVA_MIME_H

// Returns the appropriate HTTP Content-Type (MIME) text by checking the file extension
const char* get_mime_type(const char *filename);

#endif // COVA_MIME_H
