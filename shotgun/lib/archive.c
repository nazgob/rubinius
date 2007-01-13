#include <zip.h>
#include "shotgun.h"
#include "string.h"
#include "cpu.h"
#include "array.h"

OBJECT archive_list_files(STATE, char *path) {
  struct zip *za;
  struct zip_stat st;
  int i, err, count;
  OBJECT ary;
  
  if((za=zip_open(path, 0, &err)) == NULL) {
    return Qnil;
  }
  
  count = zip_get_num_files(za);
  ary = array_new(state, count);
  for(i=0; i < count; i++) {
    zip_stat_index(za, i, 0, &st);
    array_set(state, ary, i, string_new(state, (char *)st.name));
  }
  
  zip_close(za);
  
  return ary;
}

OBJECT archive_get_file(STATE, char *path, char* name) {
  struct zip *za;
  struct zip_stat st;
  struct zip_file *zf;
  OBJECT str;
  int n, total, err, file;
  char *buf;
  
  if((za=zip_open(path, 0, &err)) == NULL) {
    return Qnil;
  }
  
  if((file = zip_name_locate(za, (const char*)name, &err)) < 0) {
    zip_close(za);    
    return Qnil;
  }
  
  if((zf=zip_fopen_index(za, file, 0)) == NULL) {
    zip_close(za);
    return Qnil;
  }
  
  zip_stat_index(za, file, 0, &st);
  total = st.size;
  str = string_new2(state, NULL, total);
  buf = string_byte_address(state, str);
  
  while((n=zip_fread(zf, buf, total)) > 0) {
    buf += n;
    total -= n;
  }
  
  zip_fclose(zf);
  zip_close(za);
  
  return str;
}

OBJECT archive_get_object(STATE, char *path, char* name) {
  struct zip *za;
  struct zip_stat st;
  struct zip_file *zf;
  char *str;
  OBJECT ret;
  int n, total, err, file = -1;
  char *buf;
  
  if((za=zip_open(path, 0, &err)) == NULL) {
    return Qnil;
  }
  
  file = zip_name_locate(za, name, &err);
  if(file < 0) {
    printf("Couldn't find %s in %s (%s/%d/%s)\n", name, path, zip_strerror(za), file, strlen(name));
    zip_close(za);
    return Qnil;
  }
  
  if((zf=zip_fopen_index(za, file, 0)) == NULL) {
    zip_close(za);
    return Qnil;
  }
  
  zip_stat_index(za, file, 0, &st);
  total = st.size;
  str = malloc(sizeof(char) * total);
  buf = str;
  
  while((n=zip_fread(zf, buf, total)) > 0) {
    buf += n;
    total -= n;
  }
  
  zip_fclose(zf);
  zip_close(za);
  
  ret = cpu_unmarshal(state, str);
  free(str);
  return ret;
}

static OBJECT add_or_replace(struct zip *za, char *name, struct zip_source *zs) {
  OBJECT ret;
  int idx = zip_name_locate(za, name, 0);
  
  if(idx < 0) {
    if(zip_add(za, name, zs) < 0) {
      ret = Qfalse;
    } else {
      ret = Qtrue;
    }
  } else {
    if(zip_replace(za, idx, zs) < 0) {
      ret = Qfalse;
    } else {
      ret = Qtrue;
    }
  }
  return ret;
}

OBJECT archive_add_file(STATE, char *path, char *name, char *file) {
  struct zip *za;
  struct zip_source *zs;
  OBJECT ret;
  int err;
  
  if((za=zip_open(path, 0, &err)) == NULL) {
    return Qnil;
  }
  
  if((zs=zip_source_file(za, file, 0, 0)) == NULL) {
    zip_close(za);
    return Qfalse;
  }
  
  ret = add_or_replace(za, name, zs);
  
  zip_source_free(zs);  
  zip_close(za);
  return ret;
}

OBJECT archive_add_object(STATE, char *path, char *name, OBJECT obj) {
  struct zip *za;
  struct zip_source *zs;
  GString *buf;
  OBJECT ret;
  int err;
  
  if((za=zip_open(path, 0, &err)) == NULL) {
    return Qnil;
  }
  
  buf = cpu_marshal_to_gstring(state, obj);
  
  zs = zip_source_buffer(za, buf->str, buf->len, 0);

  ret = add_or_replace(za, name, zs);
    
  g_string_free(buf, 1);
  zip_source_free(zs);  
  zip_close(za);
  return ret;
}

OBJECT archive_delete_file(STATE, char *path, int idx) {
  struct zip *za;
  OBJECT ret;
  int err;
  
  if((za=zip_open(path, 0, &err)) == NULL) {
    return Qnil;
  }
    
  if(zip_delete(za, idx) < 0) {
    ret = Qfalse;
  } else {
    ret = Qtrue;
  }
  
  zip_close(za);
  return ret;
}

