#include <stdio.h>
#include "pdftypes.h"
#include "readpdf.h"

typedef struct file_stream_s file_stream;

struct file_stream_s
{
  int (*reset)(sub_stream*);
  int (*read)(sub_stream*, unsigned char *, int);
  pdf_parser *p; // global object
  int offset;
  int len;
  int r; // to seek
};

static int
file_seek(int off)
{
  return fseek(pdf_parser_inst.infile, off, SEEK_SET);
}
static int
file_read(unsigned char *buf, int len)
{
  return fread(buf, 1, len, pdf_parser_inst.infile);
}
static int
file_close()
{
  return 0;
}
static unsigned char*
file_cache(int len)
{
  return 0;
}

static int
fs_reset(sub_stream* s)
{
  file_stream *fs = (file_stream*)s;
  if (!fs)
    return 0;
  if (!fs->p)
    return 0;
  if (fs->r)
    {
      fs->r = 0;
      return (fs->p->seek)(fs->offset);
    }
  return 0;
}
static int
fs_read(sub_stream* s, unsigned char *buf, int len)
{
  file_stream *fs = (file_stream*)s;
  if (!fs)
    return 0;
  if (!fs->p)
    return 0;
  (fs->reset)(s);
  return (fs->p->read)(buf, len);
}

static
sub_stream*
file_stream_new(int pos, int len)
{
  file_stream *f;
  f = pdf_malloc(sizeof(file_stream));
  if (!f)
    return NULL;
  f->reset = fs_reset;
  f->read = fs_read;
  f->offset = pos;
  f->len = len;
  f->p = &pdf_parser_inst;
  f->r = 1;
  return (sub_stream*)f;
}

void
init_filestream_parser_instance(pdf_parser *p)
{
  if (!p)
    return;
   p->infile = stdin;
   p->outfile = stdout;
   p->seek = file_seek;
   p->read = file_read;
   p->close = file_close;
   p->cache = file_cache;
   p->create_stream = file_stream_new;
   p->lock = 0;
}

///////////////////////////////////////////////////////
typedef struct in_mem_stream_s in_mem_stream;

struct in_mem_stream_s
{
  int (*reset)(sub_stream*);
  int (*read)(sub_stream*, unsigned char *, int);
  unsigned char *s; // start
  unsigned char *p; // current
  unsigned char *e; // end
};

static int
im_reset(sub_stream* s)
{
  in_mem_stream *ms = (in_mem_stream*)s;
  if (!ms)
    return 0;
  return 0;
}
static int
im_read(sub_stream* s, unsigned char *buf, int len)
{
  in_mem_stream *ms = (in_mem_stream*)s;
  if (!ms)
    return 0;
  return 0;
}

static
sub_stream*
in_mem_stream_new(int pos, int len)
{
  in_mem_stream *s;
  s = pdf_malloc(sizeof(in_mem_stream));
  if (!s)
    return NULL;
  s->reset = im_reset;
  s->read = im_read;
  return (sub_stream*)s;
}

void
init_in_mem_stream_parser_instance(pdf_parser *p)
{
  if (!p)
    return;
   p->create_stream = in_mem_stream_new;
}
