#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bplustree.h"
#include "pdfindex.h"
#include "pdfmem.h"

typedef struct pdf_map
{
   int generation;
   bpt_tree *head; 
   struct pdf_map * next;
} pdf_map;

static pdf_map a_pdf_map;

pdf_map *
pdf_map_create()
{
   pdf_map * m = pdf_malloc(sizeof(pdf_map));
   memset(m, 0, sizeof(pdf_map));
   return m;
}

pdf_map *
pdf_map_insert(int n, int gen)
{
   pdf_map * m = &a_pdf_map;
   while (m->generation != gen)
   {
      m = m->next;
   }
   if (!m)
   {
      m = pdf_map_create();
   }
   if (!m->head)
   {
      m->head = bpt_new_tree(n);
   }
   return m;
}

pdf_map *
pdf_map_find(int gen)
{
   return &a_pdf_map;
}

int
pdf_obj_insert(int n, int gen, void *d)
{
  pdf_map *m = pdf_map_insert(n, gen);
   bpt_insert(m->head, n, d);
   return 0;
}

pdf_obj *
pdf_obj_find(int n, int gen)
{
   pdf_map *m = pdf_map_find(gen);
   return (pdf_obj*)bpt_search(m->head, n);
}

void
pdf_obj_walk()
{
   pdf_map * m = &a_pdf_map;
   for (;m; m = m->next)
   {
     //printf("walk generation %d...\n", m->generation);
      bpt_walk(m->head);
   }
}

void pdf_obj_delete(pdf_obj *o);

void free_array(pdf_obj *o)
{
  if (o && o->t == eArray)
    {
      int i;
      for (i = 0; i < o->value.a.len; i++)
	{
	  pdf_obj_delete(&o->value.a.items[i]);
	}
      pdf_free(o->value.a.items);
    }
}

// Remove a single obj
void
pdf_obj_delete(pdf_obj *o)
{
  if (!o)
    return;
  switch(o->t)
    {
    case eDict:
      dict_free(o->value.d.dict);
      break;
    case eString:
      pdf_free(o->value.s.buf);
      break;
    case eKey:
      pdf_free(o->value.k);
      break;
    case eArray:
      free_array(o);
      break;
    default:
      break;
    }
}

void
pdf_obj_free()
{
   pdf_map * m = &a_pdf_map;
   pdf_map *i = m;
   for (; i!=0; i=i->next)
   {
      /* delete obj tree nodes */
      bpt_delete_node(i->head);
   }
   i = m;
   for (; i!=0; i=i->next)
   {
      /* delete obj tree */
      bpt_destroy(i->head);
      pdf_free(i->head);
   }
}

