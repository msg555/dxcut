/*
Copyright (C) 2010 Mark Gordon

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <dxcut/cc.h>

#include "common.h"

using namespace std;

namespace dxcut {

ccDexValue::ccDexValue() {
  val_annotation = NULL;
}

ccDexValue::ccDexValue(const ccDexValue& x) {
  *this = x;
}

ccDexValue::~ccDexValue() {
  if(val_annotation) delete val_annotation;
}

ccDexValue& ccDexValue::operator=(const ccDexValue& x) {
  type = x.type;
  val_byte = x.val_byte;
  val_short = x.val_short;
  val_char = x.val_char;
  val_int = x.val_int;
  val_long = x.val_long;
  val_float = x.val_float;
  val_double = x.val_double;
  val_str = x.val_str;
  val_type = x.val_type;
  val_defining_class = x.val_defining_class;
  val_name = x.val_name;
  val_prototype = x.val_prototype;
  val_array = x.val_array;
  val_annotation = x.val_annotation ? new ccDexAnnotation(*x.val_annotation) :
                                      NULL;
  val_boolean = x.val_boolean;
  return *this;
}

ccDexTryBlock::ccDexTryBlock() {
  catch_all_handler = NULL;
}

ccDexTryBlock::ccDexTryBlock(const ccDexTryBlock& x) {
  *this = x;
}

ccDexTryBlock::~ccDexTryBlock() {
  if(catch_all_handler) delete catch_all_handler;
}

ccDexTryBlock& ccDexTryBlock::operator=(const ccDexTryBlock& x) {
  start_addr = x.start_addr;
  insn_count = x.insn_count;
  handlers = x.handlers;
  catch_all_handler = x.catch_all_handler ?
      new ccDexHandler(*x.catch_all_handler) : NULL;
  return *this;
}

ccDexCode::ccDexCode() {
  debug_information = NULL;
}

ccDexCode::ccDexCode(const ccDexCode& x) {
  *this = x;
}

ccDexCode::~ccDexCode() {
  if(debug_information) delete debug_information;
}

ccDexCode& ccDexCode::operator=(const ccDexCode& x) {
  registers_size = x.registers_size;
  ins_size = x.ins_size;
  outs_size = x.outs_size;
  debug_information = x.debug_information ?
      new ccDexDebugInfo(*x.debug_information) : NULL;
  tries = x.tries;
  insns = x.insns;
  return *this;
}

ccDexMethod::ccDexMethod() {
  code_body = NULL;
}

ccDexMethod::ccDexMethod(const ccDexMethod& x) {
  *this = x;
}

ccDexMethod::~ccDexMethod() {
  if(code_body) delete code_body;
}

ccDexMethod& ccDexMethod::operator=(const ccDexMethod& x) {
  access_flags = x.access_flags;
  name = x.name;
  prototype = x.prototype;
  code_body = x.code_body ? new ccDexCode(*x.code_body) : NULL;
  annotations = x.annotations;
  parameter_annotations = x.parameter_annotations;
  return *this;
}

ccDexFile::ccDexFile() {
  metadata = NULL;
}

ccDexFile::ccDexFile(const ccDexFile& x) {
  *this = x;
}

ccDexFile::~ccDexFile() {
  if(metadata) delete metadata;
}

ccDexFile& ccDexFile::operator=(const ccDexFile& x) {
  classes = x.classes;
  metadata = x.metadata ? new ccOdexData(*x.metadata) : NULL;
  return *this;
}

static
void fill_str_array(vector<string>& v, const ref_str*const* sa) {
  for(const ref_str*const* s = sa; *s; s++) {
    v.push_back((*s)->s);
  }
}

template<typename ccT, typename T>
static void fill_array(std::vector<ccT>& A, const T* v,
                       int(*sentinel_func)(const T*),
                       void(*fill_func)(ccT&, const T*)) {
  int sz = 0;
  while(!sentinel_func(v + sz)) sz++;
  A.resize(sz);
  for(int i = 0; i < sz; i++) {
    fill_func(A[i], v + i);
  }
}

static
void fill_value(ccDexValue& ccval, const DexValue* val);

static
void fill_parameter(ccDexNameValuePair& ccnvp, const DexNameValuePair* nvp) {
  ccnvp.name = nvp->name->s;
  fill_value(ccnvp.value, &nvp->value);
}

static
void fill_annotation(ccDexAnnotation& ccan, const DexAnnotation* an) {
  ccan.visibility = an->visibility;
  ccan.type = an->type->s;
  fill_array(ccan.parameters, an->parameters, dxc_is_sentinel_parameter,
             fill_parameter);
}

static
void fill_value(ccDexValue& ccval, const DexValue* val) {
  ccval.type = val->type;
  ccval.val_annotation = NULL;
  switch(val->type) {
    case VALUE_BYTE: ccval.val_byte = val->value.val_byte; break;
    case VALUE_SHORT: ccval.val_short = val->value.val_short; break;
    case VALUE_CHAR: ccval.val_char = val->value.val_char; break;
    case VALUE_INT: ccval.val_int = val->value.val_int; break;
    case VALUE_LONG: ccval.val_long = val->value.val_long; break;
    case VALUE_FLOAT: ccval.val_float = val->value.val_float; break;
    case VALUE_DOUBLE: ccval.val_double = val->value.val_double; break;
    case VALUE_STRING: ccval.val_str = val->value.val_str->s; break;
    case VALUE_TYPE: ccval.val_type = val->value.val_type->s; break;
    case VALUE_FIELD:
      ccval.val_defining_class = val->value.val_field.defining_class->s;
      ccval.val_type = val->value.val_field.type->s;
      ccval.val_name = val->value.val_field.name->s;
      break;
    case VALUE_METHOD:
      ccval.val_defining_class = val->value.val_method.defining_class->s;
      fill_str_array(ccval.val_prototype, val->value.val_method.prototype->s);
      ccval.val_name = val->value.val_method.name->s;
      break;
    case VALUE_ENUM:
      ccval.val_defining_class = val->value.val_enum.defining_class->s;
      ccval.val_type = val->value.val_enum.type->s;
      ccval.val_name = val->value.val_enum.name->s;
      break;
    case VALUE_ARRAY:
      fill_array(ccval.val_array, val->value.val_array, dxc_is_sentinel_value,
                 fill_value);
      break;
    case VALUE_ANNOTATION:
      ccval.val_annotation = new ccDexAnnotation;
      fill_annotation(*ccval.val_annotation, val->value.val_annotation);
      break;
    case VALUE_BOOLEAN:
      ccval.val_boolean = val->value.val_boolean;
      break;
  }
}

static
void fill_field(ccDexField& ccfield, const DexField* field) {
  ccfield.access_flags = field->access_flags;
  ccfield.type = field->type->s;
  ccfield.name = field->name->s;
  fill_array(ccfield.annotations, field->annotations,
             dxc_is_sentinel_annotation, fill_annotation);
}

static
void fill_handler(ccDexHandler& cchandler, const DexHandler* handler) {
  cchandler.type = handler->type ? handler->type->s : "";
  cchandler.addr = handler->addr;
}

static
void fill_try_block(ccDexTryBlock& cctr, const DexTryBlock* tr) {
  cctr.start_addr = tr->start_addr;
  cctr.insn_count = tr->insn_count;
  fill_array(cctr.handlers, tr->handlers, dxc_is_sentinel_handler,
             fill_handler);
  if(tr->catch_all_handler) {
    cctr.catch_all_handler = new ccDexHandler;
    fill_handler(*cctr.catch_all_handler, tr->catch_all_handler);
  } else {
    cctr.catch_all_handler = NULL;
  }
}

static
void fill_debug_insn(ccDexDebugInstruction& ccdebug,
                     const DexDebugInstruction* debug) {
  ccdebug.opcode = debug->opcode;
  switch(debug->opcode) {
    case DBG_ADVANCE_PC: ccdebug.addr_diff = debug->p.addr_diff; break;
    case DBG_ADVANCE_LINE: ccdebug.line_diff = debug->p.line_diff; break;
    case DBG_START_LOCAL_EXTENDED:
      ccdebug.sig = debug->p.start_local->sig->s; // no break intentional
    case DBG_START_LOCAL:
      ccdebug.register_num = debug->p.start_local->register_num;
      ccdebug.name = debug->p.start_local->name->s;
      ccdebug.type = debug->p.start_local->type->s;
      break;
    case DBG_END_LOCAL:
    case DBG_RESTART_LOCAL:
      ccdebug.register_num = debug->p.register_num;
      break;
    case DBG_SET_FILE:
      ccdebug.name = debug->p.name->s;
      break;
  }
}

static
void fill_debug_info(ccDexDebugInfo& ccdebug, const DexDebugInfo* debug) {
  ccdebug.line_start = debug->line_start;
  fill_str_array(ccdebug.parameter_names, debug->parameter_names->s);
  for(int i = 0; ; i++) {
    ccdebug.insns.resize(ccdebug.insns.size() + 1);
    fill_debug_insn(ccdebug.insns.back(), debug->insns + i);
    if(ccdebug.insns[i].opcode == DBG_END_SEQUENCE) {
      break;
    }
  }
}

static
void fill_insn(ccDexInstruction& ccin, const DexInstruction* in) {
  ccin.opcode = in->opcode;
  ccin.hi_byte = in->hi_byte;
  if(ccin.opcode == OP_PSUEDO && ccin.hi_byte != PSUEDO_OP_NOP) {
    switch(ccin.hi_byte) {
      case PSUEDO_OP_PACKED_SWITCH:
        ccin.first_key = in->special.packed_switch.first_key;
        for(int i = 0; i < in->special.packed_switch.size; i++) {
          ccin.targets.push_back(in->special.packed_switch.targets[i]);
        }
        break;
      case PSUEDO_OP_SPARSE_SWITCH:
        for(int i = 0; i < in->special.sparse_switch.size; i++) {
          ccin.keys.push_back(in->special.sparse_switch.keys[i]);
          ccin.targets.push_back(in->special.sparse_switch.targets[i]);
        }
        break;
      case PSUEDO_OP_FILL_DATA_ARRAY:
        ccin.element_width = in->special.fill_data_array.element_width;
        for(int i = 0; i < in->special.fill_data_array.element_width *
                           in->special.fill_data_array.size; i++) {
          ccin.data.push_back(in->special.fill_data_array.data[i]);
        }
        break;
      default:
        DXC_ERROR("unknown psuedo opcode");
    }
  } else switch(dex_opcode_formats[ccin.opcode].specialType) {
    case SPECIAL_NONE:
      break;
    case SPECIAL_CONSTANT:
      ccin.constant = in->special.constant;
    case SPECIAL_TARGET:
      ccin.target = in->special.target;
      break;
    case SPECIAL_STRING:
      ccin.str = in->special.str->s;
      break;
    case SPECIAL_TYPE:
      ccin.type = in->special.type->s;
      break;
    case SPECIAL_FIELD:
      ccin.defining_class = in->special.field.defining_class->s;
      ccin.name = in->special.field.name->s;
      ccin.type = in->special.field.type->s;
      break;
    case SPECIAL_METHOD:
      ccin.defining_class = in->special.method.defining_class->s;
      ccin.name = in->special.method.name->s;
      fill_str_array(ccin.prototype, in->special.method.prototype->s);
      break;
    case SPECIAL_INLINE:
      ccin.offset = in->special.inline_ind;
      break;
    case SPECIAL_OBJECT:
      ccin.offset = in->special.object_off;
      break;
    case SPECIAL_VTABLE:
      ccin.offset = in->special.vtable_ind;
      break;
    default:
      DXC_ERROR("unkonwn special format");
  }
  memcpy(ccin.param, in->param, sizeof(ccin.param));
}

static
void fill_code_body(ccDexCode& cccode, const DexCode* code) {
  cccode.registers_size = code->registers_size;
  cccode.ins_size = code->ins_size;
  cccode.outs_size = code->outs_size;
  if(code->debug_information) {
    cccode.debug_information = new ccDexDebugInfo;
    fill_debug_info(*cccode.debug_information, code->debug_information);
  } else {
    cccode.debug_information = NULL;
  }
  fill_array(cccode.tries, code->tries, dxc_is_sentinel_try_block,
             fill_try_block);
  cccode.insns.resize(code->insns_count);
  for(int i = 0; i < code->insns_count; i++) {
    fill_insn(cccode.insns[i], code->insns + i);
  }
}

static
void fill_method(ccDexMethod& ccmethod, const DexMethod* method) {
  ccmethod.access_flags = method->access_flags;
  ccmethod.name = method->name->s;
  fill_str_array(ccmethod.prototype, method->prototype->s);
  if(method->code_body) {
    ccmethod.code_body = new ccDexCode;
    fill_code_body(*ccmethod.code_body, method->code_body);
  } else {
    ccmethod.code_body = NULL;
  }
  fill_array(ccmethod.annotations, method->annotations,
             dxc_is_sentinel_annotation, fill_annotation);
  for(DexAnnotation** ptr = method->parameter_annotations; *ptr; ptr++) {
    ccmethod.parameter_annotations.resize(
        ccmethod.parameter_annotations.size() + 1);
    fill_array(ccmethod.parameter_annotations.back(), *ptr,
               dxc_is_sentinel_annotation, fill_annotation);
  }
}

static
void fill_class(ccDexClass& cccl, const DexClass* cl) {
  cccl.name = cl->name->s;
  cccl.access_flags = cl->access_flags;
  cccl.super_class = cl->super_class ? cl->super_class->s : "";
  fill_str_array(cccl.interfaces, cl->interfaces->s);
  cccl.source_file = cl->source_file ? cl->source_file->s : "";
  fill_array(cccl.annotations, cl->annotations, dxc_is_sentinel_annotation,
             fill_annotation);
  fill_array(cccl.static_values, cl->static_values, dxc_is_sentinel_value,
             fill_value);
  fill_array(cccl.static_fields, cl->static_fields, dxc_is_sentinel_field,
             fill_field);
  fill_array(cccl.instance_fields, cl->instance_fields, dxc_is_sentinel_field,
             fill_field);
  fill_array(cccl.direct_methods, cl->direct_methods, dxc_is_sentinel_method,
             fill_method);
  fill_array(cccl.virtual_methods, cl->virtual_methods, dxc_is_sentinel_method,
             fill_method);
}

static
void fill_metadata(ccOdexData& ccomd, const OdexData* omd) {
  for(int i = 0; i < 20; i++) ccomd.id.push_back(omd->id[i]);
  ccomd.flags = omd->flags;
  ccomd.odex_version = omd->odex_version;
  ccomd.dex_mod_time = omd->dex_mod_time;
  ccomd.dex_crc = omd->dex_crc;
  ccomd.vm_version = omd->vm_version;
  fill_str_array(ccomd.deps, omd->deps->s);
  int dep_sz = ccomd.deps.size();
  ccomd.dep_shas.resize(dep_sz);
  for(int i = 0; i < dep_sz; i++) {
    for(int j = 0; j < 20; j++) {
      ccomd.dep_shas[i].push_back(omd->dep_shas[i][j]);
    }
  }
  ccomd.aux_format = omd->aux_format;
  ccomd.has_class_lookup = omd->has_class_lookup;
  ccomd.has_register_maps = omd->has_register_maps;
  ccomd.has_reducing_index_map = omd->has_reducing_index_map;
  ccomd.has_expanding_index_map = omd->has_expanding_index_map;
}

static
void fill_cc(ccDexFile& ccf, const DexFile* f) {
  fill_array(ccf.classes, f->classes, dxc_is_sentinel_class, fill_class);
  if(f->metadata) {
    ccf.metadata = new ccOdexData;
    fill_metadata(*ccf.metadata, f->metadata);
  }
}

template<typename ccT, typename T>
static T* copy_array(const std::vector<ccT>& A, void(*make_sentinel_func)(T*),
                     void(*copy_func)(const ccT&, T*)) {
  T* array = (T*)calloc(A.size() + 1, sizeof(T));
  for(int i = 0; i < A.size(); i++) {
    copy_func(A[i], array + i);
  }
  make_sentinel_func(array + A.size());
  return array;
}

static
void copy_str(const string& ccs, ref_str** s) {
  *s = dxc_induct_str(ccs.c_str());
}

static
void copy_str_null(const string& ccs, ref_str** s) {
  if(ccs.empty()) {
    *s = NULL;
  } else {
    copy_str(ccs, s);
  }
}

static
ref_strstr* copy_str_array(const std::vector<std::string>& A) {
  ref_strstr* ret = dxc_create_strstr(A.size());
  for(int i = 0; i < A.size(); i++) {
    ret->s[i] = dxc_induct_str(A[i].c_str());
  }
  return ret;
}

static
void make_sentinel_str(ref_str** s) {
  *s = NULL;
}

static
void copy_annotation(const ccDexAnnotation& ccan, DexAnnotation* an);

static
void copy_value(const ccDexValue& ccval, DexValue* val) {
  val->type = ccval.type;
  val->value.val_annotation = NULL;
  switch(ccval.type) {
    case VALUE_BYTE: val->value.val_byte = ccval.val_byte; break;
    case VALUE_SHORT: val->value.val_short = ccval.val_short; break;
    case VALUE_CHAR: val->value.val_char = ccval.val_char; break;
    case VALUE_INT: val->value.val_int = ccval.val_int; break;
    case VALUE_LONG: val->value.val_long = ccval.val_long; break;
    case VALUE_FLOAT: val->value.val_float = ccval.val_float; break;
    case VALUE_DOUBLE: val->value.val_double = ccval.val_double; break;
    case VALUE_STRING: copy_str(ccval.val_str, &val->value.val_str); break;
    case VALUE_TYPE: copy_str(ccval.val_type, &val->value.val_type); break;
    case VALUE_FIELD:
      copy_str(ccval.val_defining_class, &val->value.val_field.defining_class);
      copy_str(ccval.val_type, &val->value.val_field.type);
      copy_str(ccval.val_name, &val->value.val_field.name);
      break;
    case VALUE_METHOD:
      copy_str(ccval.val_defining_class, &val->value.val_method.defining_class);
      copy_str(ccval.val_name, &val->value.val_method.name);
      val->value.val_method.prototype = copy_str_array(ccval.val_prototype);
      break;
    case VALUE_ENUM:
      copy_str(ccval.val_defining_class, &val->value.val_enum.defining_class);
      copy_str(ccval.val_type, &val->value.val_enum.type);
      copy_str(ccval.val_name, &val->value.val_enum.name);
      break;
    case VALUE_ARRAY:
      val->value.val_array = copy_array(ccval.val_array,
                                        dxc_make_sentinel_value, copy_value);
      break;
    case VALUE_ANNOTATION:
      val->value.val_annotation =
          (DexAnnotation*)calloc(1, sizeof(DexAnnotation));
      copy_annotation(*ccval.val_annotation, val->value.val_annotation);
      break;
    case VALUE_BOOLEAN:
      val->value.val_boolean = ccval.val_boolean;
      break;
  }
}

static
void copy_parameter(const ccDexNameValuePair& ccnvp, DexNameValuePair* nvp) {
  copy_str(ccnvp.name, &nvp->name);
  copy_value(ccnvp.value, &nvp->value);
}

static
void copy_annotation(const ccDexAnnotation& ccan, DexAnnotation* an) {
  an->visibility = ccan.visibility;
  copy_str(ccan.type, &an->type);
  an->parameters = copy_array(ccan.parameters, dxc_make_sentinel_parameter,
                              copy_parameter);
}

static
void copy_field(const ccDexField& ccfld, DexField* fld) {
  fld->access_flags = ccfld.access_flags;
  copy_str(ccfld.type, &fld->type);
  copy_str(ccfld.name, &fld->name);
  fld->annotations = copy_array(ccfld.annotations, dxc_make_sentinel_annotation,
                                copy_annotation);
}

static
void copy_debug_insn(const ccDexDebugInstruction& ccdebug,
                     DexDebugInstruction* debug) {
  debug->opcode = ccdebug.opcode;
  switch(ccdebug.opcode) {
    case DBG_ADVANCE_PC: debug->p.addr_diff = ccdebug.addr_diff; break;
    case DBG_ADVANCE_LINE: debug->p.line_diff = ccdebug.line_diff; break;
    case DBG_START_LOCAL_EXTENDED:
    case DBG_START_LOCAL:
      debug->p.start_local =
        (typeof(debug->p.start_local))calloc(1, sizeof(*debug->p.start_local));
      if(ccdebug.opcode == DBG_START_LOCAL_EXTENDED) {
        copy_str_null(ccdebug.sig, &debug->p.start_local->sig);
      }
      debug->p.start_local->register_num = ccdebug.register_num;
      copy_str_null(ccdebug.name, &debug->p.start_local->name);
      copy_str_null(ccdebug.type, &debug->p.start_local->type);
      break;
    case DBG_END_LOCAL:
    case DBG_RESTART_LOCAL:
      debug->p.register_num = ccdebug.register_num;
      break;
    case DBG_SET_FILE:
      copy_str_null(ccdebug.name, &debug->p.name);
      break;
  }
}

static
void copy_debug_info(const ccDexDebugInfo& ccdbg_info, DexDebugInfo* dbg_info) {
  dbg_info->line_start = ccdbg_info.line_start;
  dbg_info->parameter_names = copy_str_array(ccdbg_info.parameter_names);
  dbg_info->insns = (DexDebugInstruction*)
      calloc(ccdbg_info.insns.size(), sizeof(DexDebugInstruction));
  for(int i = 0; i < ccdbg_info.insns.size(); i++) {
    copy_debug_insn(ccdbg_info.insns[i], dbg_info->insns + i);
  }
}

static
void copy_handler(const ccDexHandler& cchnd, DexHandler* hnd) {
  copy_str_null(cchnd.type, &hnd->type);
  hnd->addr = cchnd.addr;
}

static
void copy_try_block(const ccDexTryBlock& cctb, DexTryBlock* tb) {
  tb->start_addr = cctb.start_addr;
  tb->insn_count = cctb.insn_count;
  tb->handlers = copy_array(cctb.handlers, dxc_make_sentinel_handler,
                            copy_handler);
  if(cctb.catch_all_handler) {
    tb->catch_all_handler = (DexHandler*)calloc(1, sizeof(DexHandler));
    copy_handler(*cctb.catch_all_handler, tb->catch_all_handler);
  } else {
    tb->catch_all_handler = NULL;
  }
}

static
void copy_instruction(const ccDexInstruction& ccin, DexInstruction* in) {
  in->opcode = ccin.opcode;
  in->hi_byte = ccin.hi_byte;
  if(ccin.opcode == OP_PSUEDO && ccin.hi_byte != PSUEDO_OP_NOP) {
    switch(ccin.hi_byte) {
      case PSUEDO_OP_PACKED_SWITCH:
        in->special.packed_switch.first_key = ccin.first_key;
        in->special.packed_switch.size = ccin.targets.size();
        in->special.packed_switch.targets =
            (dx_int*)malloc(sizeof(dx_int) * ccin.targets.size());
        for(int i = 0; i < ccin.targets.size(); i++) {
          in->special.packed_switch.targets[i] = ccin.targets[i];
        }
        break;
      case PSUEDO_OP_SPARSE_SWITCH:
        in->special.sparse_switch.size = ccin.targets.size();
        in->special.sparse_switch.keys =
            (dx_int*)malloc(sizeof(dx_int) * ccin.targets.size());
        in->special.sparse_switch.targets =
            (dx_int*)malloc(sizeof(dx_int) * ccin.targets.size());
        for(int i = 0; i < ccin.targets.size(); i++) {
          in->special.sparse_switch.keys[i] = ccin.keys[i];
          in->special.sparse_switch.targets[i] = ccin.targets[i];
        }
        break;
      case PSUEDO_OP_FILL_DATA_ARRAY:
        in->special.fill_data_array.element_width = ccin.element_width;
        in->special.fill_data_array.size =
            ccin.data.size() / ccin.element_width;
        in->special.fill_data_array.data =
            (dx_ubyte*)malloc(sizeof(dx_uint) * ccin.data.size());
        for(int i = 0; i < ccin.data.size(); i++) {
          in->special.fill_data_array.data[i] = ccin.data[i];
        }
        break;
      default:
        DXC_ERROR("unknown pusedo opcode");
    }
  } else switch(dex_opcode_formats[ccin.opcode].specialType) {
    case SPECIAL_NONE:
      break;
    case SPECIAL_CONSTANT:
      in->special.constant = ccin.constant;
      break;
    case SPECIAL_TARGET:
      in->special.target = ccin.target;
      break;
    case SPECIAL_STRING:
      copy_str(ccin.str, &in->special.str);
      break;
    case SPECIAL_TYPE:
      copy_str(ccin.type, &in->special.type);
      break;
    case SPECIAL_FIELD:
      copy_str(ccin.defining_class, &in->special.field.defining_class);
      copy_str(ccin.name, &in->special.field.name);
      copy_str(ccin.type, &in->special.field.type);
      break;
    case SPECIAL_METHOD:
      copy_str(ccin.defining_class, &in->special.method.defining_class);
      copy_str(ccin.name, &in->special.method.name);
      in->special.method.prototype = copy_str_array(ccin.prototype);
      break;
    case SPECIAL_INLINE:
      in->special.inline_ind = ccin.offset;
      break;
    case SPECIAL_OBJECT:
      in->special.object_off = ccin.offset;
      break;
    case SPECIAL_VTABLE:
      in->special.vtable_ind = ccin.offset;
      break;
    default:
      DXC_ERROR("unknown special format");
  }
  memcpy(in->param, ccin.param, sizeof(ccin.param));
}

static
void copy_code(const ccDexCode& cccode, DexCode* code) {
  code->registers_size = cccode.registers_size;
  code->ins_size = cccode.ins_size;
  code->outs_size = cccode.outs_size;
  if(cccode.debug_information) {
    code->debug_information =
        (DexDebugInfo*)calloc(1, sizeof(DexDebugInfo));
    copy_debug_info(*cccode.debug_information, code->debug_information);
  } else {
    code->debug_information = NULL;
  }
  code->tries = copy_array(cccode.tries, dxc_make_sentinel_try_block,
                           copy_try_block);
  code->insns_count = cccode.insns.size();
  code->insns =
      (DexInstruction*)calloc(cccode.insns.size(), sizeof(DexInstruction));
  for(int i = 0; i < cccode.insns.size(); i++) {
    copy_instruction(cccode.insns[i], code->insns + i);
  }
}

static
void make_sentinel_annotation_list(DexAnnotation** an) {
  *an = NULL;
}

static
void copy_annotation_list(const vector<ccDexAnnotation>& ccan,
                          DexAnnotation** an) {
  *an = copy_array(ccan, dxc_make_sentinel_annotation, copy_annotation);
}

static
void copy_method(const ccDexMethod& ccmtd, DexMethod* mtd) {
  mtd->access_flags = ccmtd.access_flags;
  copy_str(ccmtd.name, &mtd->name);
  mtd->prototype = copy_str_array(ccmtd.prototype);
  if(ccmtd.code_body) {
    mtd->code_body = (DexCode*)calloc(1, sizeof(DexCode));
    copy_code(*ccmtd.code_body, mtd->code_body);
  } else {
    mtd->code_body = NULL;
  }
  mtd->annotations = copy_array(ccmtd.annotations, dxc_make_sentinel_annotation,
                                copy_annotation);
  mtd->parameter_annotations = copy_array(ccmtd.parameter_annotations,
                                          make_sentinel_annotation_list,
                                          copy_annotation_list);
}

static
void copy_class(const ccDexClass& cccl, DexClass* cl) {
  copy_str(cccl.name, &cl->name);
  cl->access_flags = cccl.access_flags;
  copy_str_null(cccl.super_class, &cl->super_class);
  cl->interfaces = copy_str_array(cccl.interfaces);
  copy_str_null(cccl.source_file, &cl->source_file);
  cl->annotations = copy_array(cccl.annotations, dxc_make_sentinel_annotation,
                               copy_annotation);
  cl->static_values = copy_array(cccl.static_values, dxc_make_sentinel_value,
                                 copy_value);
  cl->static_fields = copy_array(cccl.static_fields, dxc_make_sentinel_field,
                                 copy_field);
  cl->instance_fields = copy_array(cccl.instance_fields,
                                   dxc_make_sentinel_field, copy_field);
  cl->direct_methods = copy_array(cccl.direct_methods, dxc_make_sentinel_method,
                                  copy_method);
  cl->virtual_methods = copy_array(cccl.virtual_methods,
                                   dxc_make_sentinel_method, copy_method);
}

static
void copy_metadata(const ccOdexData& ccomd, OdexData* omd) {
  memcpy(omd->id = (dx_ubyte*)malloc(20), &ccomd.id[0], 20);
  omd->flags = ccomd.flags;
  omd->odex_version = ccomd.odex_version;
  omd->dex_mod_time = ccomd.dex_mod_time;
  omd->dex_crc = ccomd.dex_crc;
  omd->vm_version = ccomd.vm_version;
  omd->deps = copy_str_array(ccomd.deps);
  omd->dep_shas = (dx_ubyte**)calloc(ccomd.deps.size() + 1, sizeof(dx_ubyte*));
  for(int i = 0; i < ccomd.deps.size(); i++) {
    memcpy(omd->dep_shas[i] = (dx_ubyte*)malloc(20), &ccomd.dep_shas[i][0], 20);
  }
  omd->aux_format = ccomd.aux_format;
  omd->has_class_lookup = ccomd.has_class_lookup;
  omd->has_register_maps = ccomd.has_register_maps;
  omd->has_reducing_index_map = ccomd.has_reducing_index_map;
  omd->has_expanding_index_map = ccomd.has_expanding_index_map;
}

static
void copy_file(const ccDexFile& ccf, DexFile* f) {
  f->classes = copy_array(ccf.classes, dxc_make_sentinel_class, copy_class);
  if(ccf.metadata) {
    f->metadata = (OdexData*)calloc(1, sizeof(OdexData));
    copy_metadata(*ccf.metadata, f->metadata);
  }
}

void ccDexValue::copy_from(const DexValue* val) {
  fill_value(*this, val);
}

void ccDexValue::copy_to(DexValue* val) const {
  copy_value(*this, val);
}

DexValue* ccDexValue::copy() const {
  DexValue* ret = (DexValue*)calloc(1, sizeof(DexValue));
  copy_value(*this, ret);
  return ret;
}

void ccDexNameValuePair::copy_from(const DexNameValuePair* nvp) {
  fill_parameter(*this, nvp);
}

void ccDexNameValuePair::copy_to(DexNameValuePair* nvp) const {
  copy_parameter(*this, nvp);
}

DexNameValuePair* ccDexNameValuePair::copy() const {
  DexNameValuePair* ret =
      (DexNameValuePair*)calloc(1, sizeof(DexNameValuePair));
  copy_parameter(*this, ret);
  return ret;
}

void ccDexAnnotation::copy_from(const DexAnnotation* an) {
  fill_annotation(*this, an);
}

void ccDexAnnotation::copy_to(DexAnnotation* an) const {
  copy_annotation(*this, an);
}

DexAnnotation* ccDexAnnotation::copy() const {
  DexAnnotation* ret = (DexAnnotation*)calloc(1, sizeof(DexAnnotation));
  copy_annotation(*this, ret);
  return ret;
}

void ccDexField::copy_from(const DexField* fld) {
  fill_field(*this, fld);
}

void ccDexField::copy_to(DexField* fld) const {
  copy_field(*this, fld);
}

DexField* ccDexField::copy() const {
  DexField* ret = (DexField*)calloc(1, sizeof(DexField));
  copy_field(*this, ret);
  return ret;
}

void ccDexDebugInstruction::copy_from(const DexDebugInstruction* dbg_insn) {
  fill_debug_insn(*this, dbg_insn);
}

void ccDexDebugInstruction::copy_to(DexDebugInstruction* dbg_insn) const {
  copy_debug_insn(*this, dbg_insn);
}

DexDebugInstruction* ccDexDebugInstruction::copy() const {
  DexDebugInstruction* ret =
      (DexDebugInstruction*)calloc(1, sizeof(DexDebugInstruction));
  copy_debug_insn(*this, ret);
  return ret;
}

void ccDexDebugInfo::copy_from(const DexDebugInfo* debug_info) {
  fill_debug_info(*this, debug_info);
}

DexDebugInfo* ccDexDebugInfo::copy() const {
  DexDebugInfo* ret = (DexDebugInfo*)calloc(1, sizeof(DexDebugInfo));
  copy_debug_info(*this, ret);
  return ret;
}

void ccDexHandler::copy_from(const DexHandler* handler) {
  fill_handler(*this, handler);
}

void ccDexHandler::copy_to(DexHandler* handler) const {
  copy_handler(*this, handler);
}

DexHandler* ccDexHandler::copy() const {
  DexHandler* ret = (DexHandler*)calloc(1, sizeof(DexHandler));
  copy_handler(*this, ret);
  return ret;
}

void ccDexTryBlock::copy_from(const DexTryBlock* tb) {
  fill_try_block(*this, tb);
}

void ccDexTryBlock::copy_to(DexTryBlock* tb) const {
  copy_try_block(*this, tb);
}

DexTryBlock* ccDexTryBlock::copy() const {
  DexTryBlock* ret = (DexTryBlock*)calloc(1, sizeof(DexTryBlock));
  copy_try_block(*this, ret);
  return ret;
}

void ccDexInstruction::copy_from(const DexInstruction* insn) {
  fill_insn(*this, insn);
}

void ccDexInstruction::copy_to(DexInstruction* insn) const {
  copy_instruction(*this, insn);
}

DexInstruction* ccDexInstruction::copy() const {
  DexInstruction* ret = (DexInstruction*)calloc(1, sizeof(DexInstruction));
  copy_instruction(*this, ret);
  return ret;
}

void ccDexCode::copy_from(const DexCode* code) {
  fill_code_body(*this, code);
}

void ccDexCode::copy_to(DexCode* code) const {
  copy_code(*this, code);
}

DexCode* ccDexCode::copy() const {
  DexCode* ret = (DexCode*)calloc(1, sizeof(DexCode));
  copy_code(*this, ret);
  return ret;
}

void ccDexMethod::copy_from(const DexMethod* mtd) {
  fill_method(*this, mtd);
}

void ccDexMethod::copy_to(DexMethod* mtd) const {
  copy_method(*this, mtd);
}

DexMethod* ccDexMethod::copy() const {
  DexMethod* ret = (DexMethod*)calloc(1, sizeof(DexMethod));
  copy_method(*this, ret);
  return ret;
}

void ccDexClass::copy_from(const DexClass* cl) {
  fill_class(*this, cl);
}

void ccDexClass::copy_to(DexClass* cl) const {
  copy_class(*this, cl);
}

DexClass* ccDexClass::copy() const {
  DexClass* ret = (DexClass*)calloc(1, sizeof(DexClass));
  copy_class(*this, ret);
  return ret;
}

void ccDexFile::copy_from(const DexFile* f) {
  fill_cc(*this, f);
}

void ccDexFile::copy_to(DexFile* f) const {
  copy_file(*this, f);
}

DexFile* ccDexFile::copy() const {
  DexFile* ret = (DexFile*)calloc(1, sizeof(DexFile));
  copy_file(*this, ret);
  return ret;
}

bool ccDexFile::read_file(FILE* f) {
  DexFile* df = dxc_read_file(f);
  if(!df) return 0;
  fill_cc(*this, df);
  dxc_free_file(df);
  return 1;
}

bool ccDexFile::read_buffer(void* buffer, dx_uint size) {
  DexFile* df = dxc_read_buffer(buffer, size);
  if(!df) return 0;
  fill_cc(*this, df);
  dxc_free_file(df);
  return 1;
}

void ccDexFile::write_file(FILE* fout) const {
  DexFile* dxf = copy();
  dxc_write_file(dxf, fout);
  dxc_free_file(dxf);
}

}
