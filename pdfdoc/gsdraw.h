#ifndef GSDRAW_H
#define GSDRAW_H

#include "pdftypes.h"

typedef enum pdf_cspacetype_e pdf_cspacetype;
typedef struct pdf_cspace_s pdf_cspace;

struct pdf_extgstate_s
{
  float LW;
  int LC;
  int LJ;
  float ML;
  int *D;
  char *RI;
  int OP;
  int op;
  int OPM;
  struct {pdf_obj * font; float size;} *Font;
  void *BG;
  void *BG2;
  void *UCR;
  void *UCR2;
  void *TR;
  void *TR2;
  float FL;
  float SM;
  int SA;
  void *BM;
  void *SMask;
  float CA;
  float ca;
  int AIS;
  int TK;
};

enum pdf_cspacetype_e
  {
    DeviceGray, DeviceRGB, DeviceCMYK, CalGray, CalRGB, Lab, ICCBased, Indexed, Pattern, Separation, DeviceN
  };

enum pdf_renderingintents_e
  {
    AbsoluteColorimetric ,
    RelativeColorimetric ,
    Saturation ,
    Perceptual ,
  };

struct pdf_calgray_s
{
  float whitepoint [3];
  float blackpoint [3];
  float Gamma ;
};

struct pdf_calrgb_s
{
  float whitepoint [3];
  float blackpoint [3];
  float Gamma ;
  float matrix[9];
};
struct pdf_lab_s
{
  float whitepoint [3];
  float blackpoint [3];
  float range[4];
};
struct pdf_icc_s
{
  int n;
  void *Alternate;
  float *range;
  void *metadata;
};

struct pdf_separation_s
{
  char *name;
  pdf_cspacetype alternatespace;
  pdf_obj *tinttransform;
};

struct pdf_indexed_s
{
  pdf_cspacetype base;
  int hival;
  void *lookup;
};

struct pdf_devicen_s
{
  char **names;
  pdf_cspacetype alternatespace;
  pdf_obj *tinttransform;
  struct {
    pdf_obj *Colorants ;
    pdf_obj *Process ;
    pdf_obj *MixingHints ;
  } *attributes;
};

struct pdf_cspace_s
{
  pdf_cspacetype t;
  void *priv;
};

#endif
