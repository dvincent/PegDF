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
#ifndef PDFTYPES_H
#define PDFTYPES_H

typedef enum pdfsimpletypes
  {
    eObjMarker,
    eDictMarker,
    eInt,
    eReal,
    eKey,
    eString,
    eArray,
    eDict,
  } e_pdfsimpletypes;

typedef struct pdftypes_s
{
  e_pdfsimpletypes t;
  union
  {
    int marker;
    int i;
    float r;
    void *k;
    void *s;
    void *a;
    void *d;
  } value;
      
}pdftypes_t;

#endif