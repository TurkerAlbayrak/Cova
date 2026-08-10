#include "multipart.h"
#include <string.h>
#include <stdio.h>

void multipart_parse(Request *req) {
    if (!req || !req->body_data || req->body_len == 0) return;
    
    // Find boundary from Content-Type header
    const char *ct = request_header(req, "Content-Type");
    if (!ct || strstr(ct, "multipart/form-data") == NULL) return;
    
    const char *b_ptr = strstr(ct, "boundary=");
    if (!b_ptr) return;
    b_ptr += 9;
    
    char boundary[128];
    snprintf(boundary, sizeof(boundary), "--%s", b_ptr);
    int b_len = strlen(boundary);
    
    unsigned char *cursor = req->body_data;
    unsigned char *end = req->body_data + req->body_len;
    
    while (cursor < end) {
        // Find boundary
        unsigned char *b_match = NULL;
        for (unsigned char *p = cursor; p <= end - b_len; p++) {
            if (memcmp(p, boundary, b_len) == 0) {
                b_match = p;
                break;
            }
        }
        
        if (!b_match) break; // no more boundaries
        
        cursor = b_match + b_len;
        if (cursor >= end || (cursor[0] == '-' && cursor[1] == '-')) {
            break; // End of multipart
        }
        
        // Skip \r\n
        if (cursor[0] == '\r' && cursor[1] == '\n') cursor += 2;
        
        // Read headers of this part
        unsigned char *headers_end = NULL;
        for (unsigned char *p = cursor; p <= end - 4; p++) {
            if (memcmp(p, "\r\n\r\n", 4) == 0) {
                headers_end = p;
                break;
            }
        }
        
        if (!headers_end) break;
        
        // Parse headers (Content-Disposition, Content-Type)
        char part_headers[1024] = {0};
        int h_len = headers_end - cursor;
        if (h_len >= (int)sizeof(part_headers)) h_len = sizeof(part_headers) - 1;
        memcpy(part_headers, cursor, h_len);
        
        char *name_ptr = strstr(part_headers, "name=\"");
        char *filename_ptr = strstr(part_headers, "filename=\"");
        char *ctype_ptr = strstr(part_headers, "Content-Type: ");
        
        cursor = headers_end + 4; // Start of binary data
        
        // Find next boundary to determine size
        unsigned char *next_b = NULL;
        for (unsigned char *p = cursor; p <= end - b_len; p++) {
            if (memcmp(p, boundary, b_len) == 0) {
                // The boundary is preceded by \r\n
                if (p >= req->body_data + 2 && p[-2] == '\r' && p[-1] == '\n') {
                    next_b = p - 2;
                } else {
                    next_b = p;
                }
                break;
            }
        }
        
        if (!next_b) next_b = end; // Just in case
        
        // Populate UploadedFile if filename exists
        if (filename_ptr && req->file_count < MAX_FILES) {
            UploadedFile *file = &req->files[req->file_count++];
            memset(file, 0, sizeof(UploadedFile));
            
            if (name_ptr) {
                name_ptr += 6;
                char *name_end = strchr(name_ptr, '"');
                if (name_end) {
                    int len = name_end - name_ptr;
                    if (len >= (int)sizeof(file->name)) len = sizeof(file->name) - 1;
                    strncpy(file->name, name_ptr, len);
                }
            }
            
            filename_ptr += 10;
            char *fname_end = strchr(filename_ptr, '"');
            if (fname_end) {
                int len = fname_end - filename_ptr;
                if (len >= (int)sizeof(file->filename)) len = sizeof(file->filename) - 1;
                strncpy(file->filename, filename_ptr, len);
            }
            
            if (ctype_ptr) {
                ctype_ptr += 14;
                char *ctype_end = strstr(ctype_ptr, "\r\n");
                if (ctype_end) {
                    int len = ctype_end - ctype_ptr;
                    if (len >= (int)sizeof(file->content_type)) len = sizeof(file->content_type) - 1;
                    strncpy(file->content_type, ctype_ptr, len);
                }
            }
            
            file->data = cursor;
            file->size = next_b - cursor;
        }
        
        cursor = next_b;
    }
}
