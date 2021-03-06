#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pdfdoc.h"
#include "pdftypes.h"
#include "pdfindex.h"
#include "pdfmem.h"
#include "readpdf.h"

static inline pdf_group*
pdf_group_load(pdf_obj *o)
{
  pdf_group *g;
  if (!o || o->t != eDict)
    return NULL;
  g = pdf_malloc(sizeof(pdf_group));
  if (!g)
    return NULL;
  memset(g, 0, sizeof(pdf_group));
  g->cs = dict_get(o->value.d.dict, "CS");
  g->i = pdf_to_int(dict_get(o->value.d.dict, "I"));
  g->k = pdf_to_int(dict_get(o->value.d.dict, "K"));
  return g;
}
pdf_err
pdf_group_free(pdf_group *g)
{
  pdf_free(g);
  return pdf_ok;
}

pdf_err pdf_page_load(pdf_obj *o, pdf_page **page)
{
  pdf_page *p;
  dict *d = o->value.d.dict;
  pdf_obj *v;

  *page = pdf_malloc(sizeof(pdf_page));
  p = *page;
  memset(p, 0, sizeof(pdf_page));
  // parse tree dict
  p->parent = dict_get(d, "Parent");
  p->mediabox = pdf_rect_resolve(dict_get(d, "MediaBox"));
  p->resources = pdf_resources_load(dict_get(d, "Resources"));
  // optionals
  p->contents = dict_get(d, "Contents");
  v = dict_get(d, "Rotate");
  if (v)
    p->rotate = v->value.i;
  p->group = pdf_group_load(dict_get(d, "Group"));
  p->annots = pdf_annots_load(dict_get(d, "Annots"));
  p->metadata = dict_get(d, "Metadata");
  p->pieceinfo = dict_get(d, "PieceInfo");
  p->lastmodified = dict_get(d, "LastModified");
  return pdf_ok;
}

pdf_err pdf_page_free(pdf_page *page)
{
  if (!page)
    return pdf_ok;
  if (page->resources)
    {
      pdf_resources_free(page->resources);
    }
  if (page->annots)
    {
      pdf_annots_free(page->annots);
    }
  if (page->group)
    {
      pdf_group_free(page->group);
    }
  pdf_free(page);
  return pdf_ok;
}

// Load into a flat array instead of tree
pdf_err pdf_page_tree_load(pdf_doc *d, pdf_obj *o)
{
  int i;
  pdf_obj *a, *kids = o;
  if (o->t == eRef)
    {
      o = pdf_obj_find(o->value.r.num, o->value.r.gen);
      a = dict_get(o->value.d.dict, "Type");
      if (!a)
	return pdf_ok;
      if (a->t == eKey && a->value.k)
        {
	  if (strcmp(a->value.k, "Pages") == 0)
            {
	      kids = dict_get(o->value.d.dict, "Kids");
	      if (!kids || ((kids->t != eArray) && (kids->t != eRef)))
		return pdf_ok;
            }
	  else if (strcmp(a->value.k, "Page") == 0)
            {
	      pdf_page_load(o, &d->pages[d->pageidx]);
	      d->pageidx += 1;
	      return pdf_ok;
            }
	  else
            {
	      return pdf_ok;
            }
        }
      else
        {
	  return pdf_ok;
        }
    }
  if (kids->t == eRef)
    {
      kids = pdf_obj_find(kids->value.r.num, kids->value.r.gen);
    }
  if (kids->t == eDict)
    {
      pdf_page_load(kids, &d->pages[d->pageidx]);
      d->pageidx += 1;
    }
  else if (kids->t == eArray)
    {
      for (i = 0; i < kids->value.a.len; i++)
        {
	  pdf_obj a;
	  a = kids->value.a.items[i];
	  if (a.t == eRef)
            {
	      pdf_page_tree_load(d, &a);
            }
        }
    }

  return pdf_ok;
}

pdf_err pdf_parse_content_stream(pdf_stream *s)
{
  int i;
  pdf_filter *f;
  unsigned char buf[16];
  if (!s)
    return pdf_ok;
  if (!s->ffilter)
    return pdf_ok;
  // construct filter train
  f = s->ffilter;
  do
    {
      i = (f->read)(f, buf, 16);
#ifdef DEBUG
      printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
#endif
    }
  while (i);
  return pdf_ok;
}

pdf_err pdf_exec_page_content(pdf_page *p)
{
    pdf_stream *c;
    if (!p)
      return pdf_ok;
    if (!p->contents)
      return pdf_ok;
    p->content_streams = pdf_streams_load(p->contents);
    for (c = p->content_streams; c; c = c->next)
    {
      pdf_parse_content_stream(c);
    }
    if (p->content_streams)
      {
	pdf_streams_free(p->content_streams);
	p->content_streams = NULL;
      }

    return pdf_ok;
}

pdf_err pdf_page_tree_walk(pdf_doc *d)
{
  int i;
  for (i = 0; i < d->count; i++)
    {
#ifdef DEBUG
      printf("processing page#%d\n", i);
#endif
      pdf_exec_page_content(d->pages[i]);
    }
  return pdf_ok;
}

pdf_doc* pdf_doc_load(pdf_obj *rdoc)
{
  pdf_obj *a, *d, *c, *kids;
  pdf_doc *doc;
  if (rdoc->t == eRef)
    {
      d = pdf_obj_find(rdoc->value.r.num, rdoc->value.r.gen);
    }
  else if (rdoc->t == eDict)
    {
      d = rdoc;
    }
  else
    {
      return pdf_ok;
    }
  if (!d || d->t != eDict)
    return NULL;
  a = dict_get(d->value.d.dict, "Pages");
  if (!a || a->t != eRef)
    return NULL;
  a = pdf_obj_find(a->value.r.num, a->value.r.gen);
  if (!a || a->t != eDict)
    return NULL;
  c = dict_get(a->value.d.dict, "Count");
  if (!c || c->t != eInt)
    return NULL;
  doc = pdf_malloc(sizeof(pdf_doc));
  if (!doc)
    return NULL;
  memset(doc, 0, sizeof(pdf_doc));
  doc->count = c->value.i;
  kids = dict_get(a->value.d.dict, "Kids");
  if (!kids || (kids->t != eArray && kids->t != eRef))
    {
      pdf_free(doc);
      return NULL;
    }
  doc->pages = pdf_malloc(sizeof(pdf_page*) * doc->count);
  memset(doc->pages, 0, (sizeof(pdf_page*) * doc->count));
  doc->pageidx = 0;
  pdf_page_tree_load(doc, kids);
  return doc;
}

pdf_err pdf_doc_process(pdf_doc *d)
{
  return pdf_page_tree_walk(d);
}

pdf_err  pdf_doc_print_info(pdf_doc *d)
{
  if (d->info)
    pdf_info_print(d->info);
  return pdf_ok;
}

void pdf_doc_done(pdf_doc *d)
{
  int i;
  for (i = 0; i < d->count; i++)
    {
      pdf_page_free(d->pages[i]);
    }
  pdf_free(d->pages);
  pdf_free(d);
}

typedef struct pdf_trailer_s pdf_trailer;
struct pdf_trailer_s
{
  int size;
  int prev;
  pdf_doc * root;
  void *encrypt;
  pdf_info *info;
  unsigned char *id[2];
};

pdf_err pdf_info_load(pdf_obj *o, pdf_info **info)
{
  pdf_obj *a;
  pdf_info *i;
  *info = pdf_malloc(sizeof(pdf_info));
  if (!*info)
    return pdf_ok;
  if (!o)
    return pdf_ok;
  memset(*info, 0, sizeof(pdf_info));
  if (o->t == eRef)
    {
      o = pdf_obj_find(o->value.r.num, o->value.r.gen);
    }
  if (!o || o->t != eDict)
    {
      pdf_free(*info);
      *info = NULL;
      return pdf_ok;
    }
  i = *info;
  a = dict_get(o->value.d.dict, "Title");
  if (a) i->title = a->value.s.buf;
  a = dict_get(o->value.d.dict, "Author");
  if (a) i->author = a->value.s.buf;
  a = dict_get(o->value.d.dict, "Subject");
  if (a) i->subject = a->value.s.buf;
  a = dict_get(o->value.d.dict, "Keywords");
  if (a) i->keywords = a->value.s.buf;
  a = dict_get(o->value.d.dict, "Creator");
  if (a) i->creator = a->value.s.buf;
  a = dict_get(o->value.d.dict, "Producer");
  if (a) i->producer = a->value.s.buf;
  a = dict_get(o->value.d.dict, "CreationDate");
  if (a) i->creationdate = a->value.s.buf;
  a = dict_get(o->value.d.dict, "ModDate");
  if (a) i->moddate = a->value.s.buf;
  a = dict_get(o->value.d.dict, "Trapped");
  i->trapped = UNknown;
  if (a)
    {
      if (strcmp(a->value.k, "True") == 0)
	i->trapped = TRue;
      else if (strcmp(a->value.k, "False") == 0)
	i->trapped = FAlse;
    }
  return pdf_ok;
}

pdf_err pdf_info_print(pdf_info *info)
{
  printf("Title:%s\n", info->title);
  printf("Auther:%s\n", info->author);
  printf("Subject:%s\n", info->subject);
  printf("Keywords:%s\n", info->keywords);
  printf("Creator:%s\n", info->creator);
  printf("Producer:%s\n", info->producer);
  printf("CreationDate:%s\n", info->creationdate);
  printf("ModDate:%s\n", info->moddate);
  printf("Trapped:%s\n", info->trapped==TRue?"Yes":(info->trapped==FAlse)?"No":"Unknown");
  return pdf_ok;
}

pdf_err pdf_trailer_open(pdf_obj *trailer)
{
  pdf_obj *a, *root;
  pdf_trailer t;
  pdf_doc * d;

  memset(&t, 0, sizeof(pdf_trailer));
  if (!trailer)
    return pdf_ok;
  if (trailer->t != eDict)
    goto done;

  root = dict_get(trailer->value.d.dict, "Root");
  if (!root || root->t != eRef)
    goto done;

  a = dict_get(trailer->value.d.dict, "Size");
  if (a && a->t == eInt)
    {
      t.size = a->value.i;
    }
  a = dict_get(trailer->value.d.dict, "Prev");
  if (a && a->t == eInt)
    {
      t.prev = a->value.i;
    }
  a = dict_get(trailer->value.d.dict, "Encrypt");
  if (a)
    {
      t.encrypt = a;
    }
  a = dict_get(trailer->value.d.dict, "Info");
  if (a)
    {
      pdf_info_load(a, &t.info);
    }
  a = dict_get(trailer->value.d.dict, "ID");
  if (a && a->t == eArray)
    {
      t.id[0] = &a->value.a.items[0];
      t.id[1] = &a->value.a.items[1];
    }
  d = pdf_doc_load(root);
  if (!d)
    goto done;
  d->info = t.info; // pass info as member of doc
  pdf_doc_process(d);
  pdf_doc_print_info(d);
  pdf_doc_done(d);
 done:
  if (t.info)
    pdf_free(t.info);
  if (trailer->t == eDict)
    dict_free(trailer->value.d.dict);
  return pdf_ok;
}

pdf_resources*
pdf_resources_load(pdf_obj *o)
{
  pdf_resources *r;
  dict *d;
  if (!o || (o->t != eDict && o->t != eRef))
    return NULL;

  if (o->t == eRef)
    {
      o = pdf_obj_find(o->value.r.num, o->value.r.gen);
    }
  r = pdf_malloc(sizeof(pdf_resources));
  if (!r)
    return NULL;
  d = o->value.d.dict;
  r->extgstate = pdf_extgstate_load(dict_get(d, "ExtGState"));
  r->colorspace = dict_get(d, "ColorSpace");
  r->pattern = dict_get(d, "Pattern");
  r->shading = dict_get(d, "Shading");
  r->xobject = dict_get(d, "XObject");
  r->font = dict_get(d, "Font");
  r->procset = dict_get(d, "ProcSet");
  r->properties = dict_get(d, "Properties");
  return r;
}

pdf_stream*
pdf_stream_load(pdf_obj* o)
{
  pdf_filter *last = NULL, *raw;
  pdf_stream *s;
  sub_stream *ss;
  pdf_obj *x, *xx, *y;
  int m, mm;
  // fill stream info
  y = o;
  if (y->t == eRef)
    y = pdf_obj_find(y->value.r.num, y->value.r.gen);
  if (!y || y->t != eDict)
    {
      return NULL;
    }
  /// internal struct. raw stream object
  ss = dict_get(y->value.d.dict, "S_O");
  if (!ss)
    return NULL;
  x = dict_get(y->value.d.dict, "Length");
  if (!x || (x->t != eInt && x->t != eRef))
    {
      fprintf(stderr, "%s\n", "Invalid stream.");
      return NULL;
    }
  s = pdf_malloc(sizeof(pdf_stream));
  if (!s)
    return NULL;
  memset(s, 0, sizeof(pdf_stream));
  x = dict_get(y->value.d.dict, "Filter");
  if (!x)
    {
      // make raw filter
      goto make_raw_filter;
    }
  else
    {
      if (x->t == eArray)
	{
	  mm = x->value.a.len;
	  xx = x->value.a.items;
	}
      else if (x->t == eKey)
	{
	  mm = 1;
	  xx = x;
	}
    }

  s->length = x->value.i;
  for (m = 0; m < mm; m++, xx++)
    {
      pdf_filterkind t = Raw;
      pdf_filter *f;
      if (xx->t != eKey)
	{
	  break;
	}
      if (strcmp(xx->value.k, "FlateDecode") == 0)
	{
	 t = FlateDecode;
	}
      else if (strcmp(xx->value.k, "ASCIIHexDecode") == 0)
	{
	  t = ASCIIHexDecode;
	}
      else if (strcmp(xx->value.k, "ASCII85Decode") == 0)
	{
	  t = ASCII85Decode;
	}
      else if (strcmp(xx->value.k, "LZWDecode") == 0)
	{
	  t = LZWDecode;
	}
      else if (strcmp(xx->value.k, "RunLengthDecode") == 0)
	{
	  t = RunLengthDecode;
	}
      else if (strcmp(xx->value.k, "CCITTFaxDecode") == 0)
	{
	  t = CCITTFaxDecode;
	}
      else if (strcmp(xx->value.k, "JBIG2Decode") == 0)
	{
	  t = JBIG2Decode;
	}
      else if (strcmp(xx->value.k, "DCTDecode") == 0)
	{
	  t = DCTDecode;
	}
      else if (strcmp(xx->value.k, "JPXDecode") == 0)
	{
	  t = JPXDecode;
	}
      else if (strcmp(xx->value.k, "Crypt") == 0)
	{
	  t = Crypt;
	}
      else
	{
	  t = Raw;
	}
      // make the filter
      f = pdf_filter_new(t);
      if (!f)
	return NULL;
      // train them
      if (!last)
	{
	  last = f;
	  s->ffilter = f;
	}
      else
	{
	  last->next = f;
	  last = f;
	}
    }
  // train the raw filter
 make_raw_filter:
  raw = pdf_rawfilter_new(ss);
  if (last)
    last->next = raw;
  else
    s->ffilter = raw;
  return s;
}

pdf_err
pdf_resources_free(pdf_resources *r)
{
  if (!r)
    return pdf_ok;
  if (r->extgstate)
    pdf_extgstate_free(r->extgstate);
  pdf_free(r);
  return pdf_ok;
}
pdf_extgstate*
pdf_extgstate_load(pdf_obj *o)
{
  pdf_extgstate *g;
  dict *d;

  if (!o || (o->t != eDict && o->t != eRef))
    return NULL;

  if (o->t == eRef)
    {
      o = pdf_obj_find(o->value.r.num, o->value.r.gen);
    }
  g = pdf_malloc(sizeof(pdf_extgstate));
  if (!g)
    return NULL;
  memset(g, 0, sizeof(pdf_extgstate));
  d = o->value.d.dict;
  // Todo: check default value
  g->LW = pdf_to_float(dict_get(d, "LW"));
  g->LC = pdf_to_int(dict_get(d, "LC"));
  g->LJ = pdf_to_int(dict_get(d, "LJ"));
  g->ML = pdf_to_float(dict_get(d, "ML"));
  g->D = pdf_to_int_array(dict_get(d, "D"));
  g->RI = pdf_to_string(dict_get(d, "RI"));
  g->OP = pdf_to_int(dict_get(d, "OP"));
  g->OPM = pdf_to_int(dict_get(d, "OPM"));
  g->op = pdf_to_int(dict_get(d, "op"));
  // font
  // bg
  g->CA = pdf_to_float(dict_get(d, "CA"));
  g->ca = pdf_to_float(dict_get(d, "ca"));
  g->AIS = pdf_to_int(dict_get(d, "AIS"));
  g->TK = pdf_to_int(dict_get(d, "TK"));
  return g;
}

pdf_err pdf_extgstate_free(pdf_extgstate*g)
{
  if (!g)
    return pdf_ok;
  pdf_free(g);
  return pdf_ok;
}

static inline char*
pdf_key_resolve(pdf_obj*o)
{
  if (!o || o->t != eKey)
    return NULL;
  return o->value.k;
}

pdf_annots*
pdf_annots_load(pdf_obj* o)
{
  pdf_annots *a, *first, *last = 0;
  dict *d;
  int i;

  if (!o || (o->t != eArray && o->t != eRef))
    return NULL;

  if (o->t == eRef)
    o = pdf_obj_find(o->value.r.num, o->value.r.gen);

  for (i = 0; i < o->value.a.len; i++)
    {
      pdf_obj *t = &o->value.a.items[i]; 
      if (t->t == eRef)
        {
	  t = pdf_obj_find(t->value.r.num, t->value.r.gen);
        }
      a = pdf_malloc(sizeof(pdf_annots));
      if (!a)
	return NULL;
      memset(a, 0, sizeof(pdf_annots));
      d = t->value.d.dict;
      // load annotation node
      a->subtype = pdf_key_resolve(dict_get(d, "SubType"));
      a->rect = pdf_rect_resolve(dict_get(d, "Rect"));
      // make linked list
      if (last)
	last->next = a;
      if (i == 0)
        {
	  first = a;
        }
      last = a;
    }
  return first;
}

pdf_err
pdf_annots_free(pdf_annots *a)
{
  pdf_annots *t;
  while (a)
    {
      t = a->next;
      pdf_free(a);
      a = t;
    }
  return pdf_ok;
}

pdf_stream*
pdf_streams_load(pdf_obj* o)
{
    pdf_stream *s, *last;
    pdf_obj *oo;
    int i, n;

    if (!o || (o->t != eDict && o->t != eRef && o->t != eArray))
        return NULL;

    if (o->t == eRef)
    {
        o = pdf_obj_find(o->value.r.num, o->value.r.gen);
    }
    if (o->t == eArray)
    {
        n = o->value.a.len;
        oo = &o->value.a.items[0];
    }
    else if (o->t == eDict)
    {
        n = 1;
        oo = o;
    }
    else
        return NULL;
    last = NULL;
    for (i = 0; i < n && oo; i++, oo++)
    {
      pdf_stream *t = pdf_stream_load(oo);
      if (last)
	{
	  last->next = t;
	}
      else
	{
	  s = t;
	}
      last = t;
    }
    return s;
}

pdf_err
pdf_streams_free(pdf_stream *s)
{
  pdf_stream *t = s;
  while (s)
    {
      t = s->next;
      pdf_stream_free(s);
      s = t;
    }
  return pdf_ok;
}

pdf_err
pdf_stream_free(pdf_stream *s)
{
  pdf_filter *f;
  if (!s)
    return pdf_ok;
  f = s->ffilter;
  while (f)
    {
      pdf_filter *t = f->next;
      (*f->close)(f);
      f = t;
    }
  pdf_free(s);
  return pdf_ok;
}
