/*
Copyright (C) 2011 by Dakai Liu

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef READPDF_H
#define READPDF_H
#include <stdio.h> // due to FILE, should be factored out ASAP
typedef struct pdf_parser_s pdf_parser;
typedef struct sub_stream_s sub_stream;

typedef struct xrefentry_s
{
  int off;
  int gen;
  char x;
} xrefentry_t;

typedef struct xreftab_s
{
  int idx;
  int count;
  xrefentry_t *obj;
} xreftab_t;

struct pdf_parser_s
{
  FILE* infile;
  FILE* outfile;
  int file_position;
  int (*seek)(int off);
  int (*read)(unsigned char *, int);
  int (*close)();
  unsigned char* (*cache)(int len);
  sub_stream* (*create_stream)(int, int);
  int lock;
};

struct sub_stream_s
{
  int (*reset)(sub_stream*);
  int (*read)(sub_stream*, unsigned char *, int);
};

extern int g_xref_off;
extern int g_xref_gen;
extern pdf_parser pdf_parser_inst;

extern int push_key(char *s);
extern int push_marker(e_pdf_kind t);
extern int push(e_pdf_kind t, int n);
extern int push_ref(e_pdf_kind t, int gen, int r);
extern pdf_obj pop(void);
extern pdf_obj pop_dict(void);
extern int pop_obj(void);
extern pdf_obj push_array(void);
extern pdf_obj push_literal(char *s);
extern void print_literal();
extern pdf_obj push_hexliteral(char *s);
extern int read_trailer(void);

extern void print_stack();
extern int xref_new(int off, int n);
extern int xref_append(pdf_obj x, int gen, int off);
extern void pop_comment(char *s, int len);
extern void pop_stream(int pos);
extern void xref_start();
extern int stream_seek(int s);
extern int stream_read(unsigned char*, int);
extern void init_filestream_parser_instance(pdf_parser *p);

#endif
