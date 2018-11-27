/* 
 * MessagePack for Matlab
 *
 * Copyright [2013] [ Yida Zhang <yida@seas.upenn.edu> ]
 *              University of Pennsylvania
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * */

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <vector>
using std::string;
using std::vector;

#include <msgpack.h>
#include "mex.h"
#include "matrix.h"

static struct flags {
  bool unicode_strs = true;
  bool pack_u8_bin = false;
  bool unpack_map_as_cells = false;
  bool unpack_narrow = false;
  bool unpack_ext_w_tag = false;
} flags;

mxArray* mex_unpack_boolean(const msgpack_object& obj);
mxArray* mex_unpack_positive_integer(const msgpack_object& obj);
mxArray* mex_unpack_negative_integer(const msgpack_object& obj);
mxArray* mex_unpack_float(const msgpack_object& obj);
mxArray* mex_unpack_double(const msgpack_object& obj);
mxArray* mex_unpack_str(const msgpack_object& obj);
mxArray* mex_unpack_nil(const msgpack_object& obj);
mxArray* mex_unpack_map(const msgpack_object& obj);
mxArray* mex_unpack_array(const msgpack_object& obj);
mxArray* mex_unpack_bin(const msgpack_object& obj);
mxArray* mex_unpack_ext(const msgpack_object& obj);

typedef struct mxArrayRes mxArrayRes;
struct mxArrayRes {
  mxArray * res;
  mxArrayRes * next;
};

#define DEFAULT_STR_SIZE 256
/* preallocate str space for unpack raw */
char *unpack_raw_str = (char *)mxMalloc(sizeof(char) * DEFAULT_STR_SIZE);

void (*PackMap[17]) (msgpack_packer *pk, int nrhs, const mxArray *prhs);
mxArray* (*unPackMap[11]) (const msgpack_object& obj);

void mexExit(void) {
//  mxFree((void *)unpack_raw_str);
  fprintf(stdout, "Existing Mex Msgpack \n");
  fflush(stdout);
}

mxArrayRes * mxArrayRes_new(mxArrayRes * head, mxArray* res) {
  mxArrayRes * new_res = (mxArrayRes *)mxCalloc(1, sizeof(mxArrayRes));
  new_res->res = res;
  new_res->next = head;
  return new_res;
}

void mxArrayRes_free(mxArrayRes * head) {
  mxArrayRes * cur_ptr = head;
  mxArrayRes * ptr = head; 
  while (cur_ptr != NULL) {
    ptr = ptr->next;
    mxFree(cur_ptr);
    cur_ptr = ptr;
  }
}

mxArray* mex_unpack_boolean(const msgpack_object& obj) {
  return mxCreateLogicalScalar(obj.via.boolean);
}

mxArray* mex_unpack_positive_integer(const msgpack_object& obj) {
  if (flags.unpack_narrow) {
    mxArray *ret = NULL;
    if ((uint8_t)obj.via.u64 == obj.via.u64) {
      // 8-bit
      ret = mxCreateNumericMatrix(1, 1, mxUINT8_CLASS, mxREAL);
      uint8_t *ptr = (uint8_t *)mxGetData(ret);
      *ptr = obj.via.u64;
    } else if ((uint16_t)obj.via.u64 == obj.via.u64) {
      // 16-bit
      ret = mxCreateNumericMatrix(1, 1, mxUINT16_CLASS, mxREAL);
      uint16_t *ptr = (uint16_t *)mxGetData(ret);
      *ptr = obj.via.u64;
    } else if ((uint32_t)obj.via.u64 == obj.via.u64) {
      // 32-bit
      ret = mxCreateNumericMatrix(1, 1, mxUINT32_CLASS, mxREAL);
      uint32_t *ptr = (uint32_t *)mxGetData(ret);
      *ptr = obj.via.u64;
    } else {
      // 64-bit
      ret = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
      uint64_t *ptr = (uint64_t *)mxGetData(ret);
      *ptr = obj.via.u64;
    }
    return ret;
  } else
    return mxCreateDoubleScalar((double)obj.via.u64);
}

mxArray* mex_unpack_negative_integer(const msgpack_object& obj) {
  if (flags.unpack_narrow) {
    mxArray *ret = NULL;
    if ((int8_t)obj.via.i64 == obj.via.i64) {
      // 8-bit
      ret = mxCreateNumericMatrix(1, 1, mxINT8_CLASS, mxREAL);
      int8_t *ptr = (int8_t *)mxGetData(ret);
      *ptr = obj.via.i64;
    } else if ((int16_t)obj.via.i64 == obj.via.i64) {
      // 16-bit
      ret = mxCreateNumericMatrix(1, 1, mxINT16_CLASS, mxREAL);
      int16_t *ptr = (int16_t *)mxGetData(ret);
      *ptr = obj.via.i64;
    } else if ((int32_t)obj.via.i64 == obj.via.i64) {
      // 32-bit
      ret = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
      int32_t *ptr = (int32_t *)mxGetData(ret);
      *ptr = obj.via.i64;
    } else {
      // 64-bit
      ret = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL);
      int64_t *ptr = (int64_t *)mxGetData(ret);
      *ptr = obj.via.i64;
    }
    return ret;
  } else
    return mxCreateDoubleScalar((double)obj.via.i64);
}

mxArray* mex_unpack_float(const msgpack_object& obj) {
  if (flags.unpack_narrow) {
    mxArray* ret = mxCreateNumericMatrix(1, 2, mxSINGLE_CLASS, mxREAL);
    float* ptr = (float *)mxGetData(ret);
    *ptr = (float)obj.via.f64;
    return ret;
  } else
    return mxCreateDoubleScalar(obj.via.f64);
}

mxArray* mex_unpack_double(const msgpack_object& obj) {
/*
  mxArray* ret = mxCreateDoubleMatrix(1,1, mxREAL);
  double *ptr = (double *)mxGetPr(ret);
  *ptr = obj.via.f64;
  return ret;
*/
  return mxCreateDoubleScalar(obj.via.f64);
}

mxArray* mex_unpack_str(const msgpack_object& obj) {
  mxArray *ret;
    mxArray* data = mxCreateNumericMatrix(1, obj.via.str.size, mxUINT8_CLASS, mxREAL);
    uint8_t *ptr = (uint8_t*)mxGetData(data);
    memcpy(ptr, obj.via.str.ptr, obj.via.str.size * sizeof(uint8_t));
  if (flags.unicode_strs) {
    // Definitely UTF-8. Convert.
    mxArray* args[] = {data, mxCreateString("UTF-8")};
    mexCallMATLAB(1, &ret, 2, args, "native2unicode");
  } else {
    // Unknown encoding. Just unpack to uint8
    ret = data;
  }
  return ret;
}

mxArray* mex_unpack_nil(const msgpack_object& obj) {
  /*
  return mxCreateCellArray(0,0);
  */
  return mxCreateDoubleScalar(0);
}

mxArray* mex_unpack_map(const msgpack_object& obj) {
  mxArray *ret = NULL;
  uint32_t nfields = obj.via.map.size;
  bool all_strs = true;
  for (size_t i = 0; i < nfields; i++) {
    if (obj.via.map.ptr[i].key.type != MSGPACK_OBJECT_STR) {
      all_strs = false;
      if (!flags.unpack_map_as_cells)
        mexWarnMsgIdAndTxt("msgpack:non_str_map_keys", "Map has non-str keys. Unpacking as 2xN cells");
      break;
    }
  }
  if (all_strs && !flags.unpack_map_as_cells) {
    // All str map keys. Unpack as struct
    char **field_name = (char **)mxCalloc(nfields, sizeof(char *));
    for (size_t i = 0; i < nfields; i++) {
      struct msgpack_object_kv obj_kv = obj.via.map.ptr[i];
      if (obj_kv.key.type == MSGPACK_OBJECT_STR) {
        /* the raw size from msgpack only counts actual characters
        * but C char array need end with \0 */
        field_name[i] = (char*)mxCalloc(obj_kv.key.via.str.size + 1, sizeof(char));
        memcpy((char*)field_name[i], obj_kv.key.via.str.ptr, obj_kv.key.via.str.size * sizeof(char));
      } else {
        mexPrintf("not string key\n");
      }
    }
    ret = mxCreateStructMatrix(1, 1, obj.via.map.size, (const char**)field_name);
    msgpack_object ob;
    for (size_t i = 0; i < nfields; i++) {
      int ifield = mxGetFieldNumber(ret, field_name[i]);
      ob = obj.via.map.ptr[i].val;
      mxSetFieldByNumber(ret, 0, ifield, (*unPackMap[ob.type])(ob));
    }
    for (size_t i = 0; i < nfields; i++)
      mxFree((void *)field_name[i]);
    mxFree(field_name);
  } else {
    // unpack as cells.
    ret = mxCreateCellMatrix(2, nfields);
    size_t key_subs[] = {0, 0};
    size_t val_subs[] = {1, 0};
    size_t key_i, val_i;
    key_i = val_i = 0;
    msgpack_object * key = NULL;
    msgpack_object * val = NULL;
    for (size_t i = 0; i < nfields; i++) {
      // Get cell indices
      key_subs[1] = i;
      key_i = mxCalcSingleSubscript(ret, 2, key_subs);
      val_subs[1] = i;
      val_i = mxCalcSingleSubscript(ret, 2, val_subs);
      key = &(obj.via.map.ptr[i].key);
      val = &(obj.via.map.ptr[i].val);
      mxSetCell(ret, key_i, (*unPackMap[key->type])(*key));
      mxSetCell(ret, val_i, (*unPackMap[val->type])(*val));
    }
  }
  return ret;
}

mxArray* mex_unpack_array(const msgpack_object& obj) {
  /* validata array element type */
  int unique_numeric_type = -1;  // msgpack_object_type
  bool one_numeric_type = true;
  int this_type;
  for (size_t i = 0; i < obj.via.array.size; i++) {
    this_type = obj.via.array.ptr[i].type;
    if (unique_numeric_type > -1) { // At least one numeric type has been found
      if (this_type > 0x00) { // Skip NIL types
        if (this_type != unique_numeric_type) {
          // Different type. Can't make an array.
          one_numeric_type = false;
          break;
        }
      }
    } else if (this_type > 0x00 && (this_type < 0x05 || this_type == 0x0a)) {
      // Found a numeric type
      unique_numeric_type = this_type;
    } else {
      // Non-numeric type. Can't make an array.
      one_numeric_type = false;
      break;
    }
  }
  if (one_numeric_type && unique_numeric_type > -1) {
    mxArray *ret = NULL;
    bool * ptrb = NULL;
    double * ptrd = NULL;
    float* ptrf = NULL;
    int64_t * ptri = NULL;
    uint64_t * ptru = NULL;
    switch (unique_numeric_type) {
      case MSGPACK_OBJECT_BOOLEAN: // 0x01
        ret = mxCreateLogicalMatrix(1, obj.via.array.size);
        ptrb = (bool*)mxGetData(ret);
        for (size_t i = 0; i < obj.via.array.size; i++) ptrb[i] = obj.via.array.ptr[i].via.boolean;
        break;
      case MSGPACK_OBJECT_POSITIVE_INTEGER: // 0x02
        ret = mxCreateNumericMatrix(1, obj.via.array.size, mxUINT64_CLASS, mxREAL);
        ptru = (uint64_t*)mxGetData(ret);
        for (size_t i = 0; i < obj.via.array.size; i++) ptru[i] = obj.via.array.ptr[i].via.u64;
        break;
      case MSGPACK_OBJECT_NEGATIVE_INTEGER: // 0x03
        ret = mxCreateNumericMatrix(1, obj.via.array.size, mxINT64_CLASS, mxREAL);
        ptri = (int64_t*)mxGetData(ret);
        for (size_t i = 0; i < obj.via.array.size; i++) ptri[i] = obj.via.array.ptr[i].via.i64;
        break;
      case MSGPACK_OBJECT_FLOAT64: // 0x04
        ret = mxCreateNumericMatrix(1, obj.via.array.size, mxDOUBLE_CLASS, mxREAL);
        ptrd = mxGetPr(ret);
        for (size_t i = 0; i < obj.via.array.size; i++) ptrd[i] = obj.via.array.ptr[i].via.f64;
        break;
      case MSGPACK_OBJECT_FLOAT32: // 0x0a
        ret = mxCreateNumericMatrix(1, obj.via.array.size, mxSINGLE_CLASS, mxREAL);
        ptrf = (float*)mxGetData(ret);
        for (size_t i = 0; i < obj.via.array.size; i++) ptrf[i] = (float)obj.via.array.ptr[i].via.f64;
        break;
      default:
        break;
    }
    return ret;
  }
  else {  // multiple types or complex type (str, array, map, bin, ext)
    mxArray *ret = mxCreateCellMatrix(1, obj.via.array.size);
    for (size_t i = 0; i < obj.via.array.size; i++) {
      msgpack_object ob = obj.via.array.ptr[i];
      mxSetCell(ret, i, (*unPackMap[ob.type])(ob));
    }
    return ret;
  }
}

mxArray* mex_unpack_bin(const msgpack_object& obj){
  mxArray* ret = mxCreateNumericMatrix(1, obj.via.bin.size, mxUINT8_CLASS, mxREAL);
  uint8_t *ptr = (uint8_t*)mxGetData(ret); 
  memcpy(ptr, obj.via.bin.ptr, obj.via.bin.size * sizeof(uint8_t));
  return ret;
}

mxArray* mex_unpack_ext(const msgpack_object& obj){
  mxArray* ret = NULL;
  int type_cell = 0;
  int data_cell = 1;
  if (flags.unpack_ext_w_tag) {
    type_cell++;
    data_cell++;
    ret = mxCreateCellMatrix(1, 3);
    mxSetCell(ret, 0, mxCreateString("MSGPACK_EXT"));
  } else {
    ret = mxCreateCellMatrix(1, 2);
  }
  mxArray* exttype = mxCreateDoubleScalar(obj.via.ext.type);
  mxArray* data = mxCreateNumericMatrix(1, obj.via.ext.size, mxUINT8_CLASS, mxREAL);
  uint8_t *ptr = (uint8_t*)mxGetData(data); 
  memcpy(ptr, obj.via.ext.ptr, obj.via.ext.size * sizeof(uint8_t));
  mxSetCell(ret, type_cell, exttype);
  mxSetCell(ret, data_cell, data);
  return ret;
}

void mex_unpack(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) 
{
  const char *str = (const char*)mxGetData(prhs[0]);
  int size = mxGetM(prhs[0]) * mxGetN(prhs[0]);

  /* deserializes it. */
  msgpack_unpacked msg;
  msgpack_unpacked_init(&msg);
  if (!msgpack_unpack_next(&msg, str, size, NULL)) 
    mexErrMsgTxt("unpack error");

  /* prints the deserialized object. */
  msgpack_object obj = msg.data;

  plhs[0] = (*unPackMap[obj.type])(obj);
}

void mex_pack_unknown(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  msgpack_pack_nil(pk);
}

void mex_pack_void(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  msgpack_pack_nil(pk);
}

void mex_pack_function(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  msgpack_pack_nil(pk);
}

void mex_pack_single(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nElements = mxGetNumberOfElements(prhs);
  float *data = (float*)mxGetData(prhs);
  if (nElements > 1) msgpack_pack_array(pk, nElements);
  for (int i = 0; i < nElements; i++) {
    msgpack_pack_float(pk, data[i]);
  }
}

void mex_pack_double(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nElements = mxGetNumberOfElements(prhs);
  double *data = mxGetPr(prhs);
  if (nElements > 1) msgpack_pack_array(pk, nElements);
  for (int i = 0; i < nElements; i++) {
    msgpack_pack_double(pk, data[i]);
  }
}

void mex_pack_int8(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nElements = mxGetNumberOfElements(prhs);
  int8_t *data = (int8_t*)mxGetData(prhs);
  if (nElements > 1) msgpack_pack_array(pk, nElements);
  for (int i = 0; i < nElements; i++) {
    msgpack_pack_int8(pk, data[i]);
  }
}

void mex_pack_uint8(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nElements = mxGetNumberOfElements(prhs);
  uint8_t *data = (uint8_t*)mxGetData(prhs);
  if (flags.pack_u8_bin) {
    msgpack_pack_bin(pk, nElements);
    msgpack_pack_bin_body(pk, data, sizeof(uint8_t)*nElements);
  } else {
    if (nElements > 1)
      msgpack_pack_array(pk, nElements);
    for (int i = 0; i < nElements; i++) {
      msgpack_pack_uint8(pk, data[i]);
    }
  }
}

void mex_pack_int16(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nElements = mxGetNumberOfElements(prhs);
  int16_t *data = (int16_t*)mxGetData(prhs);
  if (nElements > 1) msgpack_pack_array(pk, nElements);
  for (int i = 0; i < nElements; i++) {
    msgpack_pack_int16(pk, data[i]);
  }
}

void mex_pack_uint16(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nElements = mxGetNumberOfElements(prhs);
  uint16_t *data = (uint16_t*)mxGetData(prhs);
  if (nElements > 1) msgpack_pack_array(pk, nElements);
  for (int i = 0; i < nElements; i++) {
    msgpack_pack_uint16(pk, data[i]);
  }
}

void mex_pack_int32(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nElements = mxGetNumberOfElements(prhs);
  int32_t *data = (int32_t*)mxGetData(prhs);
  if (nElements > 1) msgpack_pack_array(pk, nElements);
  for (int i = 0; i < nElements; i++) {
    msgpack_pack_int32(pk, data[i]);
  }
}

void mex_pack_uint32(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nElements = mxGetNumberOfElements(prhs);
  uint32_t *data = (uint32_t*)mxGetData(prhs);
  if (nElements > 1) msgpack_pack_array(pk, nElements);
  for (int i = 0; i < nElements; i++) {
    msgpack_pack_uint32(pk, data[i]);
  }
}

void mex_pack_int64(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nElements = mxGetNumberOfElements(prhs);
  int64_t *data = (int64_t*)mxGetData(prhs);
  if (nElements > 1) msgpack_pack_array(pk, nElements);
  for (int i = 0; i < nElements; i++) {
    msgpack_pack_int64(pk, data[i]);
  }
}

void mex_pack_uint64(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nElements = mxGetNumberOfElements(prhs);
  uint64_t *data = (uint64_t*)mxGetData(prhs);
  if (nElements > 1) msgpack_pack_array(pk, nElements);
  for (int i = 0; i < nElements; i++) {
    msgpack_pack_uint64(pk, data[i]);
  }
}

void mex_pack_logical(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nElements = mxGetNumberOfElements(prhs);
  bool *data = mxGetLogicals(prhs);
  if (nElements > 1) msgpack_pack_array(pk, nElements);
  for (int i = 0; i < nElements; i++)
    (data[i])? msgpack_pack_true(pk) : msgpack_pack_false(pk);
}

void mex_pack_char(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  mwSize str_len = 0;
  char * buf = NULL;
  if (flags.unicode_strs) {
    const mxArray * args[] = {prhs, mxCreateString("UTF-8")};
    mxArray * bytes;
    mexCallMATLAB(1, &bytes, 2, (mxArray **)args, "unicode2native");
    str_len = mxGetNumberOfElements(bytes);
    char * ptr = (char *)mxGetData(bytes);
    buf = (char *)mxCalloc(str_len, sizeof(char));
    memcpy(buf, ptr, str_len);
  } else {
    str_len = mxGetNumberOfElements(prhs);
    buf = (char *)mxCalloc(str_len, sizeof(char));
    mxChar * ptr = (mxChar *)mxGetData(prhs);
    for (size_t i = 0; i < str_len; i++) {
      if ((ptr[i] >> 8) != 0)
        mexErrMsgIdAndTxt("msgpack:pack_char_data_loss",
                          "Could not unpack char>255 %c (%d).", ptr[i], ptr[i]);
      buf[i] = ptr[i];
    }
  }
  msgpack_pack_str(pk, str_len);
  msgpack_pack_str_body(pk, buf, str_len);

  mxFree(buf);
}

void mex_pack_cell(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  size_t nElements = mxGetNumberOfElements(prhs);
  if (nElements == 3) {
    mxArray* ext_tag = mxGetCell(prhs, 0);
    mxArray* ext_code = mxGetCell(prhs, 1);
    mxArray* ext_data = mxGetCell(prhs, 2);
    if (mxIsChar(ext_tag) && (strcmp(mxArrayToString(ext_tag), "MSGPACK_EXT") == 0) &&
        mxIsNumeric(ext_code) && mxIsScalar(ext_code) &&
        mxIsUint8(ext_data)) {
      int8_t code = mxGetScalar(ext_code);
      if (code != mxGetScalar(ext_code)) {
        mexErrMsgIdAndTxt("msgpack:invalid_ext_code",
                          "ext code must be integral in range [-127, 128]");
      }
      uint8_t* ptr = (uint8_t*)mxGetData(ext_data);
      size_t len = mxGetNumberOfElements(ext_data);
      msgpack_pack_ext(pk, len, code);
      msgpack_pack_ext_body(pk, ptr, len*sizeof(uint8_t));
      return;
    }
  }
  if (nElements > 1) msgpack_pack_array(pk, nElements);
  for (size_t i = 0; i < nElements; i++) {
    mxArray * pm = mxGetCell(prhs, i);
    (*PackMap[mxGetClassID(pm)])(pk, nrhs, pm);
  }
}

void mex_pack_struct(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nField = mxGetNumberOfFields(prhs);
  if (nField > 1) msgpack_pack_map(pk, nField);
  const char* fname = NULL;
  int fnameLen = 0;
  int ifield = 0;
  for (int i = 0; i < nField; i++) {
    fname = mxGetFieldNameByNumber(prhs, i);
    fnameLen = strlen(fname);
    msgpack_pack_str(pk, fnameLen);
    msgpack_pack_str_body(pk, fname, fnameLen);
    ifield = mxGetFieldNumber(prhs, fname);
    mxArray* pm = mxGetFieldByNumber(prhs, 0, ifield);
    (*PackMap[mxGetClassID(pm)])(pk, nrhs, pm);
  }
}

void mex_pack(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  /* creates buffer and serializer instance. */
  msgpack_sbuffer* buffer = msgpack_sbuffer_new();
  msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  for (int i = 0; i < nrhs; i ++)
    (*PackMap[mxGetClassID(prhs[i])])(pk, nrhs, prhs[i]);

  plhs[0] = mxCreateNumericMatrix(1, buffer->size, mxUINT8_CLASS, mxREAL);
  memcpy((uint8_t*)mxGetData(plhs[0]), buffer->data, buffer->size * sizeof(uint8_t));

  /* cleaning */
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(pk);
}

void mex_pack_raw(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  /* creates buffer and serializer instance. */
  msgpack_sbuffer* buffer = msgpack_sbuffer_new();
  msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  for (int i = 0; i < nrhs; i ++) {
    size_t nElements = mxGetNumberOfElements(prhs[i]);
    size_t sElements = mxGetElementSize(prhs[i]);
    uint8_t *data = (uint8_t*)mxGetData(prhs[i]);
    msgpack_pack_str(pk, nElements * sElements);
    msgpack_pack_str_body(pk, data, nElements * sElements);
  }

  plhs[0] = mxCreateNumericMatrix(1, buffer->size, mxUINT8_CLASS, mxREAL);
  memcpy((uint8_t*)mxGetData(plhs[0]), buffer->data, buffer->size * sizeof(uint8_t));

  /* cleaning */
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(pk);
}

void mex_unpacker_set_cell(mxArray *plhs, int nlhs, mxArrayRes *res) {
  if (nlhs > 0)
    mex_unpacker_set_cell(plhs, nlhs-1, res->next);
  mxSetCell(plhs, nlhs, res->res);
}

void mex_unpacker(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  mxArrayRes * ret = NULL;
  int npack = 0;
  /* Init deserialize using msgpack_unpacker */
  msgpack_unpacker pac;
  msgpack_unpacker_init(&pac, MSGPACK_UNPACKER_INIT_BUFFER_SIZE);

  const char *str = (const char*)mxGetData(prhs[0]);
  int size = mxGetM(prhs[0]) * mxGetN(prhs[0]);
  if (size) {
    /* feeds the buffer */
    msgpack_unpacker_reserve_buffer(&pac, size);
    memcpy(msgpack_unpacker_buffer(&pac), str, size);
    msgpack_unpacker_buffer_consumed(&pac, size);
  
    /* start streaming deserialization */
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    for (;msgpack_unpacker_next(&pac, &msg); npack++) {
      /* prints the deserialized object. */
      msgpack_object obj = msg.data;
      ret = mxArrayRes_new(ret, (*unPackMap[obj.type])(obj));
    }
    /* set cell for output */
    plhs[0] = mxCreateCellMatrix(npack, 1);
    mex_unpacker_set_cell((mxArray *)plhs[0], npack-1, ret);
  }

  mxArrayRes_free(ret);
}

vector<mxArray *> cells;
void mex_unpacker_std(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  int npack = 0;
  /* Init deserialize using msgpack_unpacker */
  msgpack_unpacker pac;
  msgpack_unpacker_init(&pac, MSGPACK_UNPACKER_INIT_BUFFER_SIZE);

  const char *str = (const char*)mxGetData(prhs[0]);
  int size = mxGetM(prhs[0]) * mxGetN(prhs[0]);
  if (size) {
    /* feeds the buffer */
    msgpack_unpacker_reserve_buffer(&pac, size);
    memcpy(msgpack_unpacker_buffer(&pac), str, size);
    msgpack_unpacker_buffer_consumed(&pac, size);
  
    /* start streaming deserialization */
    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    for (;msgpack_unpacker_next(&pac, &msg); npack++) {
      /* prints the deserialized object. */
      msgpack_object obj = msg.data;
      cells.push_back((*unPackMap[obj.type])(obj));
    }
    /* set cell for output */
    plhs[0] = mxCreateCellMatrix(1, npack);
    for (size_t i = 0; i < cells.size(); i++)
      mxSetCell(plhs[0], i, cells[i]);
    cells.clear();
  }
}

void split_string(vector<string>& result, const string& str, char delim=' ') {
  result.clear();
  std::stringstream ss(str);
  while (ss.good()) {
    string substr;
    getline(ss, substr, delim);
    result.push_back(substr);
  }
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  static bool init = false;
  /* Init unpack functions Map */
  if (!init) {
    unPackMap[MSGPACK_OBJECT_NIL] = mex_unpack_nil;
    unPackMap[MSGPACK_OBJECT_BOOLEAN] = mex_unpack_boolean;
    unPackMap[MSGPACK_OBJECT_POSITIVE_INTEGER] = mex_unpack_positive_integer;
    unPackMap[MSGPACK_OBJECT_NEGATIVE_INTEGER] = mex_unpack_negative_integer;
    unPackMap[MSGPACK_OBJECT_FLOAT32] = mex_unpack_float;
    unPackMap[MSGPACK_OBJECT_FLOAT64] = mex_unpack_double;
    unPackMap[MSGPACK_OBJECT_STR] = mex_unpack_str;
    unPackMap[MSGPACK_OBJECT_ARRAY] = mex_unpack_array;
    unPackMap[MSGPACK_OBJECT_MAP] = mex_unpack_map;
    unPackMap[MSGPACK_OBJECT_BIN] = mex_unpack_bin;
    unPackMap[MSGPACK_OBJECT_EXT] = mex_unpack_ext;

    PackMap[mxUNKNOWN_CLASS] = mex_pack_unknown;
    PackMap[mxVOID_CLASS] = mex_pack_void;
    PackMap[mxFUNCTION_CLASS] = mex_pack_function;
    PackMap[mxCELL_CLASS] = mex_pack_cell;
    PackMap[mxSTRUCT_CLASS] = mex_pack_struct;
    PackMap[mxLOGICAL_CLASS] = mex_pack_logical;
    PackMap[mxCHAR_CLASS] = mex_pack_char;
    PackMap[mxDOUBLE_CLASS] = mex_pack_double;
    PackMap[mxSINGLE_CLASS] = mex_pack_single;
    PackMap[mxINT8_CLASS] = mex_pack_int8;
    PackMap[mxUINT8_CLASS] = mex_pack_uint8;
    PackMap[mxINT16_CLASS] = mex_pack_int16;
    PackMap[mxUINT16_CLASS] = mex_pack_uint16;
    PackMap[mxINT32_CLASS] = mex_pack_int32;
    PackMap[mxUINT32_CLASS] = mex_pack_uint32;
    PackMap[mxINT64_CLASS] = mex_pack_int64;
    PackMap[mxUINT64_CLASS] = mex_pack_uint64;

    mexAtExit(mexExit);
    init = true;
  }

  if ((nrhs < 1) || (!mxIsChar(prhs[0])))
    mexErrMsgTxt("Need to input string argument");
  string cmd_string(mxArrayToString(prhs[0]));
  // Split cmd_string into "<cmd> [flags]"
  vector<string> flag_vec;
  split_string(flag_vec, cmd_string);
  string cmd(flag_vec[0]);
  // Process flags
  for (vector<string>::iterator it = flag_vec.begin()+1; it != flag_vec.end(); ++it) {
    if (*it == "+pack_u8_bin") flags.pack_u8_bin = true;
    else if (*it == "-pack_u8_bin") flags.pack_u8_bin = false;
    else if (*it == "+unicode_strs") flags.unicode_strs = true;
    else if (*it == "-unicode_strs") flags.unicode_strs = false;
    else if (*it == "+unpack_map_as_cells") flags.unpack_map_as_cells = true;
    else if (*it == "-unpack_map_as_cells") flags.unpack_map_as_cells = false;
    else if (*it == "+unpack_narrow") flags.unpack_narrow = true;
    else if (*it == "-unpack_narrow") flags.unpack_narrow = false;
    else if (*it == "+unpack_ext_w_tag") flags.unpack_ext_w_tag = true;
    else if (*it == "-unpack_ext_w_tag") flags.unpack_ext_w_tag = false;
    else mexErrMsgIdAndTxt("msgpack:invalid_flag", "%s is not a valid flag.", it->c_str());
  }
  // Handle command
  if (cmd == "setflags") {
    // flags already processed above
    return;
  } else if (cmd == "pack")
    mex_pack(nlhs, plhs, nrhs-1, prhs+1);
  else if (cmd == "unpack")
    mex_unpack(nlhs, plhs, nrhs-1, prhs+1);
  else if (cmd == "unpacker")
    mex_unpacker_std(nlhs, plhs, nrhs-1, prhs+1);
  else if (cmd == "help") {
    mexPrintf("See README.md\n");
  }
//  else if (cmd == "unpacker_std")
//    mex_unpacker_std(nlhs, plhs, nrhs-1, prhs+1);
  else
    mexErrMsgTxt("Unknown function argument");
}

