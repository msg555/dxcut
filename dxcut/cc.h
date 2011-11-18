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
#ifndef __DXCUT_CC_H
#define __DXCUT_CC_H

#include <vector>
#include <string>

#include <dxcut/cdxcut>

namespace dxcut {

class ccDexAnnotation;

class ccDexValue {
 public:
  ccDexValue();
  ccDexValue(const ccDexValue&);
  ~ccDexValue();
  ccDexValue& operator=(const ccDexValue&);

  DexValueType type;
  dx_byte val_byte;
  dx_short val_short;
  dx_ushort val_char;
  dx_int val_int;
  dx_long val_long;
  float val_float;
  double val_double;
  std::string val_str;
  std::string val_type; // VALUE_TYPE, VALUE_FIELD, VALUE_ENUM
  std::string val_defining_class; // VALUE_FIELD, VALUE_METHOD, VALUE_ENUM
  std::string val_name; // VALUE_FIELD, VALUE_METHOD, VALUE_ENUM
  std::vector<std::string> val_prototype; // VALUE_METHOD
  std::vector<ccDexValue> val_array;
  ccDexAnnotation* val_annotation; // NULL if not present, allocate with new.
                                   // MUST be present if type is
                                   // VALUE_ANNOATATION
  bool val_boolean;

  void copy_from(const DexValue* val);
  void copy_to(DexValue* val) const;
  DexValue* copy() const;
};

class ccDexNameValuePair {
 public:
  std::string name;
  ccDexValue value;

  void copy_from(const DexNameValuePair* nvp);
  void copy_to(DexNameValuePair* nvp) const;
  DexNameValuePair* copy() const;
};

class ccDexAnnotation {
 public:
  DexAnnotationVisibility visibility;
  std::string type;
  std::vector<ccDexNameValuePair> parameters;

  void copy_from(const DexAnnotation* an);
  void copy_to(DexAnnotation* an) const;
  DexAnnotation* copy() const;
};

class ccDexField {
 public:
  DexAccessFlags access_flags;
  std::string type;
  std::string name;
  std::vector<ccDexAnnotation> annotations;

  void copy_from(const DexField* fld);
  void copy_to(DexField* fld) const;
  DexField* copy() const;
};

class ccDexDebugInstruction {
 public:
  dx_ubyte opcode;
  dx_uint addr_diff;
  dx_uint line_diff;
  dx_uint register_num;
  std::string name;
  std::string type;
  std::string sig;

  void copy_from(const DexDebugInstruction* dbg_insn);
  void copy_to(DexDebugInstruction* dbg_insn) const;
  DexDebugInstruction* copy() const;
};

class ccDexDebugInfo {
 public:
  dx_uint line_start;
  std::vector<std::string> parameter_names;
  std::vector<ccDexDebugInstruction> insns;

  void copy_from(const DexDebugInfo* debug_info);
  void copy_to(DexDebugInfo* debug_info) const;
  DexDebugInfo* copy() const;
};

class ccDexHandler {
 public:
  std::string type; // Should be empty string for catch all handlers.
  dx_uint addr;

  void copy_from(const DexHandler* handler);
  void copy_to(DexHandler* handler) const;
  DexHandler* copy() const;
};

class ccDexTryBlock {
 public:
  ccDexTryBlock();
  ccDexTryBlock(const ccDexTryBlock&);
  ~ccDexTryBlock();
  ccDexTryBlock& operator=(const ccDexTryBlock&);

  dx_uint start_addr;
  dx_ushort insn_count;
  std::vector<ccDexHandler> handlers;
  ccDexHandler* catch_all_handler; // NULL if not present, allocate with new.

  void copy_from(const DexTryBlock* tb);
  void copy_to(DexTryBlock* tb) const;
  DexTryBlock* copy() const;
};

class ccDexInstruction {
 public:
  dx_ubyte opcode;
  dx_ubyte hi_byte;
  dx_ulong constant;
  dx_int target;
  std::string str;
  std::string defining_class; // SPECIAL_FIELD, SPECIAL_METHOD
  std::string name; // SPECIAL_FIELD, SPECIAL_METHOD
  std::string type; // SPECIAL_TYPE, SPECIAL_FIELD
  std::vector<std::string> prototype; // SPECIAL_METHOD
  dx_uint offset; // SPECIAL_INLINE, SPECIAL_OBJECT, SPECIAL_VTABLE
  dx_int first_key; // PACKED_SWITCH
  std::vector<dx_int> keys; // SPARSE_SWITCH
  std::vector<dx_int> targets; // PACKED_SWITCH
  dx_ushort element_width; // FILL_ARRAY_DATA
  std::vector<dx_ubyte> data; // FILL_ARRAY_DATA
  dx_ushort param[2];

  void copy_from(const DexInstruction* insn);
  void copy_to(DexInstruction* insn) const;
  DexInstruction* copy() const;
};

class ccDexCode {
 public:
  ccDexCode();
  ccDexCode(const ccDexCode&);
  ~ccDexCode();
  ccDexCode& operator=(const ccDexCode&);

  dx_ushort registers_size;
  dx_ushort ins_size;
  dx_ushort outs_size;
  ccDexDebugInfo* debug_information; // NULL if not present, allocate with new.
  std::vector<ccDexTryBlock> tries;
  std::vector<ccDexInstruction> insns;

  void copy_from(const DexCode* code);
  void copy_to(DexCode* code) const;
  DexCode* copy() const;
};

class ccDexMethod {
 public:
  ccDexMethod();
  ccDexMethod(const ccDexMethod&);
  ~ccDexMethod();
  ccDexMethod& operator=(const ccDexMethod&);

  DexAccessFlags access_flags;
  std::string name;
  std::vector<std::string> prototype;
  ccDexCode* code_body; // NULL if not present, allocate with new.
  std::vector<ccDexAnnotation> annotations;
  std::vector<std::vector<ccDexAnnotation> > parameter_annotations;

  void copy_from(const DexMethod* mtd);
  void copy_to(DexMethod* mtd) const;
  DexMethod* copy() const;
};

class ccDexClass {
 public:
  std::string name;
  DexAccessFlags access_flags;
  std::string super_class;
  std::vector<std::string> interfaces;
  std::string source_file;
  std::vector<ccDexAnnotation> annotations;
  std::vector<ccDexValue> static_values;
  std::vector<ccDexField> static_fields;
  std::vector<ccDexField> instance_fields;
  std::vector<ccDexMethod> direct_methods;
  std::vector<ccDexMethod> virtual_methods;

  void copy_from(const DexClass* cl);
  void copy_to(DexClass* cl) const;
  DexClass* copy() const;
};

class ccOdexData {
 public:
  std::vector<dx_ubyte> id;
  dx_uint flags;
  dx_uint odex_version;
  dx_uint dex_mod_time;
  dx_uint dex_crc;
  dx_uint vm_version;
  std::vector<std::string> deps;
  std::vector<std::vector<dx_ubyte> > dep_shas;
  dx_uint aux_format;
  dx_uint has_class_lookup;
  dx_uint has_register_maps;
  dx_uint has_reducing_index_map;
  dx_uint has_expanding_index_map;
};

class ccDexFile {
 public:
  ccDexFile();
  ccDexFile(const ccDexFile&);
  ~ccDexFile();
  ccDexFile& operator=(const ccDexFile&);

  std::vector<ccDexClass> classes;
  ccOdexData* metadata;

  void copy_from(const DexFile* f);
  void copy_to(DexFile* f) const;
  DexFile* copy() const;

  bool read_file(FILE* f);
  bool read_buffer(void* buffer, dx_uint size);
  void write_file(FILE* fout) const;
};

}

#endif // __DXCUT_CC_H
