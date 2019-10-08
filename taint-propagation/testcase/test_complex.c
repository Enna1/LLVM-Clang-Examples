#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define get_uint16(bptr) (((bptr)[0] << 8) | (bptr)[1])

typedef uint8_t byte;

typedef struct
{
    byte *buf;
    size_t buf_size;
    unsigned int buf_rd_ix;
    unsigned int buf_wr_ix;
} Jbig2Ctx;

typedef struct
{
    byte flags;
    uint32_t HGW;
    uint32_t HGH;
    int32_t HGX;
    int32_t HGY;
    uint16_t HRX;
    uint16_t HRY;
    bool HMMR;
    int HTEMPLATE;
    bool HENABLESKIP;
    bool HDEFPIXEL;
} Jbig2HalftoneRegionParams;

uint32_t jbig2_get_uint32(const byte *bptr)
{
    return ((uint32_t)get_uint16(bptr) << 16) | get_uint16(bptr + 2);
}

void jbig2_image_new(Jbig2Ctx *ctx, int width, int height)
{
    int stride;
    int64_t check;
    stride = ((width - 1) >> 3) + 1;
    check = ((int64_t)stride) * ((int64_t)height);
    byte *b = malloc(sizeof(uint8_t) * ((int)check + 1));
}

void jbig2_decode_gray_scale_image(Jbig2Ctx *ctx, uint32_t GSW, uint32_t GSH)
{
    jbig2_image_new(ctx, GSW, GSH);
}

void jbig2_decode_halftone_region(Jbig2Ctx *ctx, Jbig2HalftoneRegionParams *params)
{
    jbig2_decode_gray_scale_image(ctx, params->HGW, params->HGH);
}

void jbig2_halftone_region(Jbig2Ctx *ctx, const byte *segment_data)
{
    int offset = 18;
    Jbig2HalftoneRegionParams params;
    params.HGW = jbig2_get_uint32(segment_data + offset);
    params.HGH = jbig2_get_uint32(segment_data + offset + 4);
    jbig2_decode_halftone_region(ctx, &params);
}

void jbig2_parse_segment(Jbig2Ctx *ctx, const uint8_t *segment_data)
{
    jbig2_halftone_region(ctx, segment_data);
}

void jbig2_data_in(Jbig2Ctx *ctx, const unsigned char *data, size_t size)
{
    byte *buf;
    size_t buf_size = 1024;
    buf = malloc(sizeof(byte) * buf_size);
    ctx->buf = buf;
    ctx->buf_rd_ix = 0;
    ctx->buf_wr_ix = 0;
    memcpy(ctx->buf + ctx->buf_wr_ix, data, size);
    jbig2_parse_segment(ctx, ctx->buf + ctx->buf_rd_ix);
}

int main(int argc, char **argv)
{
    Jbig2Ctx ctx;
    uint8_t buf[4096];
    FILE *f = fopen(argv[2], "rb");
    int n_bytes = fread(buf, 1, sizeof(buf), f);
    jbig2_data_in(&ctx, buf, n_bytes);
    return 0;
}