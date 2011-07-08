#include <dxcut/dex.h>
#include <dxcut/util.h>

#include <stdlib.h>
#include <string.h>

typedef struct RenameContext {
  int num_fields;
  ref_field* source_fields;
  ref_field* dest_fields;
  
  int num_methods;
  ref_method* source_methods;
  ref_method* dest_methods;

  int num_classes;
  ref_str** source_classes;
  ref_str** dest_classes;
} RenameContext;

static
int compare_fields(ref_field* a, ref_field* b) {
  int r = strcmp(a->defining_class->s, b->defining_class->s);
  if(!r) r = strcmp(a->name->s, b->name->s);
  if(!r) r = strcmp(a->type->s, b->type->s);
  return r;
}

static
int compare_methods(ref_method* a, ref_method* b) {
  int r = strcmp(a->defining_class->s, b->defining_class->s);
  if(!r) r = strcmp(a->name->s, b->name->s);
  if(!r) {
    ref_str** pa = a->prototype->s;
    ref_str** pb = b->prototype->s;
    while(!r && *pa && *pb) {
      r = strcmp((*pa++)->s, (*pb++)->s);
    }
    if(!r && *pb) r = -1;
    if(!r && *pa) r = 1;
  }
  return r;
}

static
int compare_classes(ref_str* a, ref_str* b) {
  return strcmp(a->s, b->s);
}

static
void dorename_field(ref_field* val, RenameContext* ctx) {
  int lo = 0;
  int hi = ctx->num_fields;
  while(lo < hi) {
    int md = lo + (hi - lo) / 2;
    int cmp = compare_fields(val, ctx->source_fields + md);
    if(!cmp) {
      dxc_free_str(val->defining_class);
      dxc_free_str(val->name);
      dxc_free_str(val->type);
      val->defining_class = dxc_copy_str(ctx->dest_fields[md].defining_class);
      val->name = dxc_copy_str(ctx->dest_fields[md].name);
      val->type = dxc_copy_str(ctx->dest_fields[md].type);
    } else if(cmp < 0) {
      hi = md;
    } else {
      lo = md + 1;
    }
  }
}

static
void dorename_method(ref_method* val, RenameContext* ctx) {
  int lo = 0;
  int hi = ctx->num_methods;
  while(lo < hi) {
    int md = lo + (hi - lo) / 2;
    int cmp = compare_methods(val, ctx->source_methods + md);
    if(!cmp) {
      dxc_free_str(val->defining_class);
      dxc_free_str(val->name);
      dxc_free_strstr(val->prototype);
      val->defining_class = dxc_copy_str(ctx->dest_methods[md].defining_class);
      val->name = dxc_copy_str(ctx->dest_methods[md].name);
      val->prototype = dxc_copy_strstr(ctx->dest_methods[md].prototype);
    } else if(cmp < 0) {
      hi = md;
    } else {
      lo = md + 1;
    }
  }
}

static
void dorename_class(ref_str** val, RenameContext* ctx) {
  if(!*val) return;

  int lo = 0;
  int hi = ctx->num_classes;
  while(lo < hi) {
    int md = lo + (hi - lo) / 2;
    int cmp = compare_classes(*val, ctx->source_classes[md]);
    if(!cmp) {
      dxc_free_str(*val);
      *val = dxc_copy_str(ctx->dest_classes[md]);
    } else if(cmp < 0) {
      hi = md;
    } else {
      lo = md + 1;
    }
  }
}

static
void rename_strstr(ref_strstr* ss, RenameContext* ctx) {
  ref_str** s;
  for(s = ss->s; *s; s++) {
    dorename_class(s, ctx);
  }
}

static
void rename_value(DexValue* val, RenameContext* ctx);

static
void rename_annotation(DexAnnotation* annotation, RenameContext* ctx) {
  int i;
  dorename_class(&annotation->type, ctx);
  for(i = 0; i < !dxc_is_sentinel_parameter(annotation->parameters + i); i++) {
    rename_value(&annotation->parameters[i].value, ctx);
  }
}

static
void rename_value(DexValue* val, RenameContext* ctx) {
  int i;
  switch(val->type) {
    case VALUE_TYPE: {
      dorename_class(&val->value.val_type, ctx);
      break;
    } case VALUE_FIELD: {
      dorename_field(&val->value.val_field, ctx);
      dorename_class(&val->value.val_field.defining_class, ctx);
      dorename_class(&val->value.val_field.type, ctx);
      break;
    } case VALUE_METHOD: {
      dorename_method(&val->value.val_method, ctx);
      dorename_class(&val->value.val_method.defining_class, ctx);
      rename_strstr(val->value.val_method.prototype, ctx);
      break;
    } case VALUE_ENUM: {
      dorename_field(&val->value.val_enum, ctx);
      dorename_class(&val->value.val_enum.defining_class, ctx);
      dorename_class(&val->value.val_enum.type, ctx);
      break;
    } case VALUE_ARRAY: {
      for(i = 0; !dxc_is_sentinel_value(val->value.val_array + i); i++) {
        rename_value(val->value.val_array + i, ctx);
      }
      break;
    } case VALUE_ANNOTATION: {
      rename_annotation(val->value.val_annotation, ctx);
      break;
    }
    case VALUE_BYTE: case VALUE_SHORT: case VALUE_INT: case VALUE_LONG:
    case VALUE_FLOAT: case VALUE_DOUBLE: case VALUE_STRING: case VALUE_NULL:
    case VALUE_BOOLEAN: case VALUE_SENTINEL: case VALUE_CHAR:
      break;
  }
}

static
void rename_field(DexField* field, RenameContext* ctx, ref_str* cl) {
  int i;

  ref_field fld;
  fld.defining_class = dxc_copy_str(cl);
  fld.name = dxc_copy_str(field->name);
  fld.type = dxc_copy_str(field->type);
  dorename_field(&fld, ctx);
  dxc_free_str(field->name);
  dxc_free_str(field->type);
  field->name = dxc_copy_str(fld.name);
  field->type = dxc_copy_str(fld.type);
  if(compare_classes(cl, fld.defining_class)) {
    fprintf(stderr, "dxc_rename_identifiers: locally definined field "
                    "has defining class changed\n");
    fflush(stderr);
  }
  dxc_free_str(fld.defining_class);
  dxc_free_str(fld.name);
  dxc_free_str(fld.type);

  dorename_class(&field->type, ctx);
  for(i = 0; !dxc_is_sentinel_annotation(field->annotations + i); i++) {
    rename_annotation(field->annotations + i, ctx);
  }
}

static
void rename_method(DexMethod* method, RenameContext* ctx, ref_str* cl) {
  dx_uint i, j;

  ref_method mtd;
  mtd.defining_class = dxc_copy_str(cl);
  mtd.name = dxc_copy_str(method->name);
  mtd.prototype = dxc_copy_strstr(method->prototype);
  dorename_method(&mtd, ctx);
  dxc_free_str(method->name);
  dxc_free_strstr(method->prototype);
  method->name = dxc_copy_str(mtd.name);
  method->prototype = dxc_copy_strstr(mtd.prototype);
  if(compare_classes(cl, mtd.defining_class)) {
    fprintf(stderr, "dxc_rename_identifiers: locally definined method "
                    "has defining class changed\n");
    fflush(stderr);
  }
  dxc_free_str(mtd.defining_class);
  dxc_free_str(mtd.name);
  dxc_free_strstr(mtd.prototype);

  rename_strstr(method->prototype, ctx);
  DexCode* code = method->code_body;
  if(code) {
    DexDebugInfo* dbg = code->debug_information;
    if(dbg) {
      for(i = 0; dbg->insns[i].opcode != DBG_END_SEQUENCE; i++) {
        if(dbg->insns[i].opcode == DBG_START_LOCAL ||
           dbg->insns[i].opcode == DBG_START_LOCAL_EXTENDED) {
          if(dbg->insns[i].p.start_local) {
            dorename_class(&dbg->insns[i].p.start_local->type, ctx);
          }
        }
      }
    }
    for(i = 0; !dxc_is_sentinel_try_block(code->tries + i); i++) {
      DexTryBlock* tb = code->tries + i;
      for(j = 0; !dxc_is_sentinel_handler(tb->handlers + j); j++) {
        dorename_class(&tb->handlers[j].type, ctx);
      }
    }
    for(i = 0; i < code->insns_count; i++) {
      DexInstruction* in = code->insns + i;
      switch(dex_opcode_formats[in->opcode].specialType) {
        case SPECIAL_TYPE: {
          dorename_class(&in->special.type, ctx);
          break;
        } case SPECIAL_FIELD: {
          dorename_field(&in->special.field, ctx);
          dorename_class(&in->special.field.defining_class, ctx);
          dorename_class(&in->special.field.type, ctx);
          break;
        } case SPECIAL_METHOD: {
          dorename_method(&in->special.method, ctx);
          dorename_class(&in->special.method.defining_class, ctx);
          rename_strstr(in->special.method.prototype, ctx);
          break;
        }
        case SPECIAL_NONE: case SPECIAL_CONSTANT: case SPECIAL_TARGET:
        case SPECIAL_STRING: case SPECIAL_INLINE: case SPECIAL_OBJECT:
        case SPECIAL_VTABLE:
          break;
      }
    }
  }
  for(i = 0; !dxc_is_sentinel_annotation(method->annotations + i); i++) {
    rename_annotation(method->annotations + i, ctx);
  }
  for(i = 0; method->parameter_annotations[i]; i++) {
    for(j = 0; !dxc_is_sentinel_annotation(
                  method->parameter_annotations[i] + j); j++) {
      rename_annotation(method->parameter_annotations[i] + j, ctx);
    }
  }
}

void dxc_rename_identifiers(DexFile* file,
      int num_fields, ref_field* source_fields, ref_field* dest_fields,
      int num_methods, ref_method* source_methods, ref_method* dest_methods,
      int num_classes, ref_str** source_classes, ref_str** dest_classes) {
  int i, j;
  RenameContext ctx;

  ctx.num_fields = num_fields;
  ctx.num_methods = num_methods;
  ctx.num_classes = num_classes;
  ctx.source_fields = (ref_field*)malloc(num_fields * sizeof(ref_field));
  ctx.dest_fields = (ref_field*)malloc(num_fields * sizeof(ref_field));
  ctx.source_methods = (ref_method*)malloc(num_methods * sizeof(ref_method));
  ctx.dest_methods = (ref_method*)malloc(num_methods * sizeof(ref_method));
  ctx.source_classes = (ref_str**)malloc(num_classes * sizeof(ref_str*));
  ctx.dest_classes = (ref_str**)malloc(num_classes * sizeof(ref_str*));

  memcpy(ctx.source_fields, source_fields, num_fields * sizeof(ref_field));
  memcpy(ctx.dest_fields, dest_fields, num_fields * sizeof(ref_field));
  memcpy(ctx.source_methods, source_methods, num_methods * sizeof(ref_method));
  memcpy(ctx.dest_methods, dest_methods, num_methods * sizeof(ref_method));
  memcpy(ctx.source_classes, source_classes, num_classes * sizeof(ref_str*));
  memcpy(ctx.dest_classes, dest_classes, num_classes * sizeof(ref_str*));
  
  for(i = 0; i < num_fields; i++) {
    for(j = i + 1; j < num_fields; j++) {
      if(compare_fields(ctx.source_fields + j, ctx.source_fields + i) < 0) {
        ref_field tmp = ctx.source_fields[i];
        ctx.source_fields[i] = ctx.source_fields[j];
        ctx.source_fields[j] = tmp;
        tmp = ctx.dest_fields[i];
        ctx.dest_fields[i] = ctx.dest_fields[j];
        ctx.dest_fields[j] = tmp;
      }
    }
  }
  for(i = 0; i < num_methods; i++) {
    for(j = i + 1; j < num_methods; j++) {
      if(compare_methods(ctx.source_methods + j, ctx.source_methods + i) < 0) {
        ref_method tmp = ctx.source_methods[i];
        ctx.source_methods[i] = ctx.source_methods[j];
        ctx.source_methods[j] = tmp;
        tmp = ctx.dest_methods[i];
        ctx.dest_methods[i] = ctx.dest_methods[j];
        ctx.dest_methods[j] = tmp;
      }
    }
  }
  for(i = 0; i < num_classes; i++) {
    for(j = i + 1; j < num_classes; j++) {
      if(compare_classes(ctx.source_classes[j], ctx.source_classes[i]) < 0) {
        ref_str* tmp = ctx.source_classes[i];
        ctx.source_classes[i] = ctx.source_classes[j];
        ctx.source_classes[j] = tmp;
        tmp = ctx.dest_classes[i];
        ctx.dest_classes[i] = ctx.dest_classes[j];
        ctx.dest_classes[j] = tmp;
      }
    }
  }

  DexClass* cl;
  for(cl = file->classes; !dxc_is_sentinel_class(cl); ++cl) {
    for(i = 0; !dxc_is_sentinel_annotation(cl->annotations + i); i++) {
      rename_annotation(cl->annotations + i, &ctx);
    }
    for(i = 0; !dxc_is_sentinel_value(cl->static_values + i); i++) {
      rename_value(cl->static_values + i, &ctx);
    }
    for(i = 0; !dxc_is_sentinel_field(cl->static_fields + i); i++) {
      rename_field(cl->static_fields + i, &ctx, cl->name);
    }
    for(i = 0; !dxc_is_sentinel_field(cl->instance_fields + i); i++) {
      rename_field(cl->instance_fields + i, &ctx, cl->name);
    }
    for(i = 0; !dxc_is_sentinel_method(cl->direct_methods + i); i++) {
      rename_method(cl->direct_methods + i, &ctx, cl->name);
    }
    for(i = 0; !dxc_is_sentinel_method(cl->virtual_methods + i); i++) {
      rename_method(cl->virtual_methods + i, &ctx, cl->name);
    }
    dorename_class(&cl->name, &ctx);
    dorename_class(&cl->super_class, &ctx);
    rename_strstr(cl->interfaces, &ctx);
  }

  free(ctx.source_fields); free(ctx.dest_fields);
  free(ctx.source_methods); free(ctx.dest_methods);
  free(ctx.source_classes); free(ctx.dest_classes);
}
