#ifndef __RUBINIUS_INCLUDED__
#define __RUBINIUS_INCLUDED__ 1

#include <stdio.h>
#include <stdlib.h>

typedef unsigned long OBJECT;

struct rubinius_object {
  unsigned char flags;
  unsigned char flags2;
  /* Used by the GC to store data about the object. */
  unsigned short int gc;
  unsigned long hash;
  
  OBJECT klass;
  unsigned long fields;  
};

#include <assert.h>

#define HEADER(obj) ((struct rubinius_object*)obj)
#define OBJECTS(obj) ((OBJECT*)obj)
#define FLAGS_OFFSET 0
#define FLAGS2_OFFSET 1
#define HASH_OFFSET 4
#define CLASS_OFFSET 8
#define FIELDS_OFFSET 12
/* Header size is in longs */
#define HEADER_SIZE 4

#define REFSIZE (sizeof(size_t))

#define CLASS_OBJECT(obj) (HEADER(obj)->klass)

/* ->fields is shifted by 1 so that the least most bit is always
    1. This protects the field from being interpretted as a reference
    if it's blindly looked at. */

#define NUM_FIELDS(obj) (HEADER(obj)->fields)
#define SET_NUM_FIELDS(obj, fel) (HEADER(obj)->fields = fel)
#define SIZE_IN_BYTES(obj) ((NUM_FIELDS(obj) + HEADER_SIZE) * REFSIZE)
#define SIZE_OF_BODY(obj) (NUM_FIELDS(obj) * REFSIZE)
#define ADDRESS_OF_FIELD(obj, fel) ((OBJECT)(OBJECTS(obj) + HEADER_SIZE + fel))
#define NTH_FIELD_DIRECT(obj, fel) (*(OBJECT*)(OBJECTS(obj) + HEADER_SIZE + fel))

#define BYTES_OF(obj) ((char*)(OBJECTS(obj) + HEADER_SIZE))
#define FIXNUM_NEG(obj) ((((unsigned long)obj) & 4) == 4)

#ifndef INTERNAL_MACROS

#define SET_FIELD(obj, fel, val) rbs_set_field(obj, fel, val)

static inline OBJECT rbs_get_field(OBJECT in, int fel) {
  OBJECT obj;
  assert(fel < HEADER(in)->fields);
  obj = NTH_FIELD_DIRECT(in, fel);
  return obj;
}

#define NTH_FIELD(obj, fel) rbs_get_field(obj, fel)

static inline OBJECT rbs_set_field(OBJECT obj, int fel, OBJECT val) {
  assert(fel < HEADER(obj)->fields);
  OBJECT *slot = (OBJECT*)ADDRESS_OF_FIELD(obj, fel);
  assert(val != 12);
  *slot = val;
  return val;
}

#endif

#ifdef Qfalse
#undef Qfalse
#undef Qtrue
#undef Qnil
#undef Qundef
#undef RTEST
#undef NIL_P
#undef SYMBOL_P
#undef FIXNUM_P
#endif

#define Qfalse ((OBJECT)0)
#define Qtrue  ((OBJECT)2)
#define Qnil   ((OBJECT)4)
#define Qundef ((OBJECT)6)

#define RTEST(v) (((OBJECT)(v) & ~Qnil) != 0)
#define NIL_P(v) ((OBJECT)(v) == Qnil)
#define REFERENCE_P(v) ({ unsigned long _i = (unsigned long)v; _i > 10 && ((_i & 1) == 0); })
#define SYMBOL_P(v) (((OBJECT)(v) & 3) == 3)
#define FIXNUM_P(v) (((OBJECT)(v) & 3) == 1)

#define FLAG_SET(obj, flag) (HEADER(obj)->flags |= flag)
#define FLAG_SET_P(obj, flag) ((HEADER(obj)->flags & flag) == flag)

#define FLAG_SET_ON(obj, fel, flag) (HEADER(obj)->fel |= flag)
#define FLAG_SET_ON_P(obj, fel, flag) ((HEADER(obj)->fel & flag) == flag)


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef STATE
#define STATE void*
#endif

#ifndef __SHOTGUN__

OBJECT rbs_const_set(STATE, OBJECT module, char *name, OBJECT obj);
OBJECT rbs_const_get(STATE, OBJECT module, char *name);
OBJECT rbs_class_new(STATE, char *name, int fields, OBJECT obj);
char *rbs_symbol_to_cstring(STATE, OBJECT sym);
char *rbs_inspect(STATE, OBJECT obj);

#endif

#ifndef INTERNAL_MACROS
static inline long rbs_to_int(OBJECT obj) {
  long val = ((unsigned long)obj) >> 3;
  if(FIXNUM_NEG(obj)) { 
    val = -val;
  }
  return val;
}

static inline OBJECT rbs_int_to_fixnum(int num) {
  OBJECT ret;
  int ab;
  ab = abs(num);
  ret = (ab << 3) | 1;

  if(num < 0) {
    ret = ret | 4;
  }
  return ret;
}

#define FIXNUM_TO_INT(obj) rbs_to_int(obj)
#define INT_TO_FIXNUM(int) rbs_int_to_fixnum(int)
#define I2N(i) INT_TO_FIXNUM(i)

#endif

#endif

