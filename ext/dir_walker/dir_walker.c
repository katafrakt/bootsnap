#include "ruby.h"
#include <dirent.h>
#include <stdlib.h>

static VALUE dw_walk(VALUE, VALUE, VALUE);
void walk_recur(char*, char**, int);
int is_prefix(const char *pre, const char *str);

static VALUE module, bootsnap_module;

void
Init_dir_walker(void)
{
  //bootsnap_module = rb_const_get(rb_cModule, rb_intern("Bootsnap"));
  bootsnap_module = rb_define_module("Bootsnap");
  module = rb_define_module_under(bootsnap_module, "DirWalker");
  rb_define_module_function(module, "walk", dw_walk, 2);
}

static VALUE
dw_walk(VALUE self, VALUE path, VALUE opts)
{
  char* c_path;
  VALUE excluded, result;
  char **exclusions;
  int num_of_excluded, str_len, i;

  excluded = rb_funcall(opts, rb_intern("[]"), 1, ID2SYM(rb_intern("excluded")));
  if(NIL_P(excluded))
  {
    excluded = rb_ary_new();
  }

  c_path = RSTRING_PTR(path);

  num_of_excluded = NUM2INT(rb_funcall(excluded, rb_intern("length"), 0));
  exclusions = malloc(num_of_excluded * sizeof(char*));
  for(i=0;i<num_of_excluded;i++)
  {
    result = rb_ary_entry(excluded, i);
    Check_Type(result, T_STRING);
    str_len = RSTRING_LEN(result);
    exclusions[i] = RSTRING_PTR(result);
  }

  walk_recur(c_path, exclusions, num_of_excluded);

  free(exclusions);

  return Qnil;
}

void walk_recur(char *base_path, char **exclusions, int num_of_exclusions)
{
    char path[4000], *abspath, formatted_str[4000];
    struct dirent *dp;
    DIR *dir = opendir(base_path);
    VALUE to_yield;
    int i, should_skip = 0;

    if (!dir)
        return;

    abspath = realpath(base_path, NULL);

    while ((dp = readdir(dir)) != NULL)
    {
      should_skip = 0;
      if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        continue;
      
      if(dp->d_name[0] == '.')
        continue;

      sprintf(formatted_str, "%s/%s", abspath, dp->d_name);

      for(i=0; i<num_of_exclusions; i++)
      {
        if(is_prefix(exclusions[i], formatted_str))
        {
          should_skip = 1;
        }
      }
      if(should_skip > 0) continue;

      to_yield = rb_str_new_cstr(formatted_str);
      rb_yield_values(1, to_yield);

      // prepare for recursion
      strcpy(path, base_path);
      strcat(path, "/");
      strcat(path, dp->d_name);

      walk_recur(path, exclusions, num_of_exclusions);
    }

    closedir(dir);
}

int is_prefix(const char *pre, const char *str)
{
    return strncmp(pre, str, strlen(pre)) == 0;
}