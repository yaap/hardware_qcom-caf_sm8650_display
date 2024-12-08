#ifndef UBWCPLIB_H
#define UBWCPLIB_H

// UBWC Image Formats
typedef enum {
    UBWCPLib_RGBA8888 = 0,  // RGBA8888 format
    UBWCPLib_NV12,          // NV12 format
    UBWCPLib_TP10,          // TP10 format
    UBWCPLib_P010,          // P010 format
    UBWCPLib_RGB,           // RGB format
    UBWCPLib_YUV,           // YUV format
    UBWCPLib_UNKNOWN,       // Unknown format
    UBWCPLib_NUM_FORMATS    // Total number of formats
} UBWCPLib_Image_Format;

// UBWC buffer attributes structure
typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int stride;
    UBWCPLib_Image_Format image_format;
    unsigned int scanlines;
    unsigned int planar_padding;
} UBWCPLib_buf_attrs;

// Function prototypes for UBWC operations
int LINK_UBWCPLib_get_stride_alignment(void *context, UBWCPLib_Image_Format format, size_t *stride);
int LINK_UBWCPLib_validate_stride(void *context, unsigned int width, UBWCPLib_Image_Format format, unsigned int height);
int LINK_UBWCPLib_set_buf_attrs(void *context, unsigned int width, UBWCPLib_buf_attrs *attrs);
void* LINK_UBWCPLib_create_session();
void LINK_UBWCPLib_destroy_session(void *session);

#endif // UBWCPLIB_H
