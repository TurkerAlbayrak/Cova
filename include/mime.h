#ifndef COVA_MIME_H
#define COVA_MIME_H

// Dosya uzantısına bakarak uygun HTTP Content-Type (MIME) metnini döndürür
const char* get_mime_type(const char *filename);

#endif // COVA_MIME_H
