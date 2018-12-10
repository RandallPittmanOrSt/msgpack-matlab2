/*
 * MessagePack for Matlab
 *
 * Origial Source Copyright [2013] [ Yida Zhang <yida@seas.upenn.edu> ]
 *              University of Pennsylvania
 * msgpack-matlab2 modifications Copyright [2018] [ Randall Pittman <randallpittman@outlook.com> ]
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
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
using std::string;
using std::vector;

#include <msgpack.h>
#include "mex.h"
#include "matrix.h"

enum NilUnpack {UNPACK_NIL_ZERO, UNPACK_NIL_NAN, UNPACK_NIL_EMPTY, UNPACK_NIL_CELL};

static struct mp_flags {
  bool unicode_strs = true;
  bool pack_u8_bin = false;
  bool unpack_map_as_cells = false;
  bool unpack_narrow = false;
  bool unpack_ext_w_tag = false;
  bool pack_other_as_nil = true;
  NilUnpack unpack_nil = UNPACK_NIL_ZERO;
  bool unpack_nil_array_skip = true;
} flags;

void print_flags() {
  mexPrintf("%cunicode_strs\n", (flags.unicode_strs) ? '+' : '-');
  mexPrintf("%cpack_u8_bin\n", (flags.pack_u8_bin) ? '+' : '-');
  mexPrintf("%cunpack_map_as_cells\n", (flags.unpack_map_as_cells) ? '+' : '-');
  mexPrintf("%cunpack_narrow\n", (flags.unpack_narrow) ? '+' : '-');
  mexPrintf("%cunpack_ext_w_tag\n", (flags.unpack_ext_w_tag) ? '+' : '-');
  mexPrintf("%cpack_other_as_nil\n", (flags.pack_other_as_nil) ? '+' : '-');
  mexPrintf("%cunpack_nil_array_skip\n", (flags.unpack_nil_array_skip) ? '+' : '-');
  mexPrintf("+unpack_nil_");
  switch (flags.unpack_nil) {
    case UNPACK_NIL_ZERO:
      mexPrintf("zero\n");
      break;
    case UNPACK_NIL_NAN:
      mexPrintf("NaN\n");
      break;
    case UNPACK_NIL_EMPTY:
      mexPrintf("empty\n");
      break;
    case UNPACK_NIL_CELL:
      mexPrintf("cell\n");
      break;
  }
}

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

// Create pack and unpack function mappings
typedef void (*pack_fn)(msgpack_packer* pk, int nrhs, const mxArray* prhs);
pack_fn PackMap[16] = {NULL};
typedef mxArray* (*unpack_fn)(const msgpack_object& obj);
unpack_fn unPackMap[11] = {NULL};

// pack and unpack wrapper functions
void pack_mxArray(msgpack_packer *pk, int nrhs, const mxArray* prhs);
mxArray* unpack_obj(const msgpack_object& obj);

void mexExit(void) {
  fprintf(stdout, "Existing Mex Msgpack \n");
  fflush(stdout);
}

mxArray* unpack_obj(const msgpack_object& obj) {
  mxArray* ret = NULL;
  if (obj.type >= 0 && obj.type <= 0x0a) {
    ret = (*unPackMap[obj.type])(obj);
  } else {
    mexErrMsgIdAndTxt("msgpack:unpack_bad_object_type",
                      "Don't know how to unpack object type %d.", obj.type);
  }
  return ret;
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
  mxArray* ret = NULL;
  switch (flags.unpack_nil) {
    case UNPACK_NIL_ZERO:
      ret = mxCreateDoubleScalar(0);
      break;
    case UNPACK_NIL_NAN:
      ret = mxCreateDoubleScalar(mxGetNaN());
      break;
    case UNPACK_NIL_EMPTY:
      ret = mxCreateDoubleMatrix(0, 0, mxREAL);
      break;
    case UNPACK_NIL_CELL:
      ret = mxCreateCellArray(0, 0);
      break;
  }
  return ret;
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
      mxSetFieldByNumber(ret, 0, ifield, unpack_obj(ob));
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
      mxSetCell(ret, key_i, unpack_obj(*key));
      mxSetCell(ret, val_i, unpack_obj(*val));
    }
  }
  return ret;
}

mxArray* mex_unpack_array(const msgpack_object& obj) {
  mxArray* ret = NULL;
  // Short circuit--empty array returns [];
  if (obj.via.array.size == 0) {
    ret = mxCreateDoubleMatrix(0, 0, mxREAL);
    return ret;
  }
  // Figure out if the array is all of one scalar type, (or one type with nils)
  int unique_scalar_type = -1;  // msgpack_object_type
  bool one_scalar_type = true;
  int this_type;
  vector<bool> nils(obj.via.array.size, false);
  for (size_t i = 0; i < obj.via.array.size; i++) {
    this_type = obj.via.array.ptr[i].type;
    if (this_type == 0x00) {
        nils[i] = true;
        continue;
    }
    if (unique_scalar_type > -1) { // At least one scalar type has been found
      if (this_type != unique_scalar_type) {
        // Different type. Can't make an array.
        one_scalar_type = false;
        break;
      }
    } else if (this_type > 0x00 && (this_type < 0x05 || this_type == 0x0a)) {
      // Found a scalar non-nil type
      unique_scalar_type = this_type;
    } else {
      // Non-scalar type. Can't make an array.
      one_scalar_type = false;
      break;
    }
  }
  bool all_nils = (unique_scalar_type == -1);
  bool any_nils = std::any_of(nils.begin(), nils.end(), [](bool v) {return v;});
  // mexPrintf("unique_scalar_type: %d, one_scalar_type: %d, all_nils: %d, any_nils: %d\n",
  //           unique_scalar_type, one_scalar_type, all_nils, any_nils);

  // Single-type array if...
  // - All scalar of same type and no nills
  // - All scalar of same type and skip nil
  // - All scalar of same type and nil-->zero
  // - All scalar of float/double type and nil-->NaN
  // - All nil and (nil-->Nan or nil-->zero)
  // ...otherwise cell array
  if ((one_scalar_type &&
       (!any_nils ||
        flags.unpack_nil_array_skip ||
        flags.unpack_nil == UNPACK_NIL_ZERO ||
        (flags.unpack_nil == UNPACK_NIL_NAN && (unique_scalar_type == 0x04 ||
                                                unique_scalar_type == 0x0a)))) ||
      (all_nils && (flags.unpack_nil == UNPACK_NIL_NAN || flags.unpack_nil == UNPACK_NIL_ZERO))){
    // Unpack to single-type MATLAB array
    // First handle the three all-nil cases.
    if (all_nils) {
      if (flags.unpack_nil_array_skip) {
        ret = mxCreateNumericMatrix(0, 0, mxDOUBLE_CLASS, mxREAL);
      } else if (flags.unpack_nil == UNPACK_NIL_ZERO) {
        ret = mxCreateNumericMatrix(1, obj.via.array.size, mxUINT8_CLASS, mxREAL);
      } else {
        ret = mxCreateNumericMatrix(1, obj.via.array.size, mxDOUBLE_CLASS, mxREAL);
        double* ptr = mxGetPr(ret);
        for (size_t i = 0; i < obj.via.array.size; i++) ptr[i] = mxGetNaN();
      }
    } else {
      // Ok, there are between 0 and n-1 nils. Create the arrays.
      size_t nskip = 0;
      if (any_nils && flags.unpack_nil_array_skip) {
        nskip = std::count_if(nils.begin(), nils.end(), [](bool v) {return v;});
      }
      bool* ptrb = NULL;
      uint64_t* ptru = NULL;
      int64_t* ptri = NULL;
      float* ptrf = NULL;
      double* ptrd = NULL;
      switch (unique_scalar_type) {
        case MSGPACK_OBJECT_BOOLEAN:
          ret = mxCreateLogicalMatrix(1, obj.via.array.size - nskip);
          ptrb = (bool*)mxGetData(ret);
          if (!any_nils) {
            // No nils, copy in bool values.
            for (size_t i = 0; i < obj.via.array.size; i++)
              ptrb[i] = obj.via.array.ptr[i].via.boolean;
          } else {
            if (flags.unpack_nil_array_skip) {
              // Some nils, skip them
              size_t ptr_i = 0;
              for (size_t obj_i = 0; obj_i < obj.via.array.size; obj_i++) {
                if (nils[obj_i]) continue;
                else ptrb[ptr_i++] = obj.via.array.ptr[obj_i].via.boolean; // incr ptr_i after use
              }
            } else {
              // some nils, set them false.
              for (size_t i = 0; i < obj.via.array.size; i++) {
                if (nils[i]) ptrb[i] = false;
                else ptrb[i] = obj.via.array.ptr[i].via.boolean;
              }
            }
          }
          break;
        case MSGPACK_OBJECT_POSITIVE_INTEGER:
          ret = mxCreateNumericMatrix(1, obj.via.array.size - nskip, mxUINT64_CLASS, mxREAL);
          ptru = (uint64_t*)mxGetData(ret);
          if (!any_nils) {
            // no nils, copy in uints
            for (size_t i = 0; i < obj.via.array.size; i++)
              ptru[i] = obj.via.array.ptr[i].via.u64;
          } else {
            if (flags.unpack_nil_array_skip) {
              // Some nils, skip them
              size_t ptr_i = 0;
              for (size_t obj_i = 0; obj_i < obj.via.array.size; obj_i++) {
                if (nils[obj_i]) continue;
                else ptru[ptr_i++] = obj.via.array.ptr[obj_i].via.u64; // incr ptr_i after use
              }
            } else {
              // some nils, set them to zero
              for (size_t i = 0; i < obj.via.array.size; i++) {
                if (nils[i]) ptru[i] = 0;
                else ptru[i] = obj.via.array.ptr[i].via.u64;
              }
            }
          }
          break;
        case MSGPACK_OBJECT_NEGATIVE_INTEGER:
          ret = mxCreateNumericMatrix(1, obj.via.array.size - nskip, mxINT64_CLASS, mxREAL);
          ptri = (int64_t*)mxGetData(ret);
          if (!any_nils) {
            // no nils, copy in ints
            for (size_t i = 0; i < obj.via.array.size; i++)
              ptri[i] = obj.via.array.ptr[i].via.i64;
          } else {
            if (flags.unpack_nil_array_skip) {
              // Some nils, skip them
              size_t ptr_i = 0;
              for (size_t obj_i = 0; obj_i < obj.via.array.size; obj_i++) {
                if (nils[obj_i]) continue;
                else ptri[ptr_i++] = obj.via.array.ptr[obj_i].via.i64; // incr ptr_i after use
              }
            } else {
              // some nils, set them to zero
              for (size_t i = 0; i < obj.via.array.size; i++) {
                if (nils[i]) ptri[i] = 0;
                else ptri[i] = obj.via.array.ptr[i].via.i64;
              }
            }
          }
          break;
        case MSGPACK_OBJECT_FLOAT32:
          ret = mxCreateNumericMatrix(1, obj.via.array.size - nskip, mxSINGLE_CLASS, mxREAL);
          ptrf = (float*)mxGetData(ret);
          if (!any_nils) {
            // no nils, copy in vals
            for (size_t i = 0; i < obj.via.array.size; i++)
              ptrf[i] = (float)obj.via.array.ptr[i].via.f64;
          } else {
            if (flags.unpack_nil_array_skip) {
              // Some nils, skip them
              size_t ptr_i = 0;
              for (size_t obj_i = 0; obj_i < obj.via.array.size; obj_i++) {
                if (nils[obj_i]) continue;
                else ptrf[ptr_i++] = (float)obj.via.array.ptr[obj_i].via.f64; // incr ptr_i after use
              }
            } else {
              // some nils, set them to zero or NaN.
              float nil_val = (flags.unpack_nil == UNPACK_NIL_NAN) ? mxGetNaN() : 0;
              for (size_t i = 0; i < obj.via.array.size; i++) {
                if (nils[i]) ptrf[i] = nil_val;
                else ptrf[i] = (float)obj.via.array.ptr[i].via.f64;
              }
            }
          }
          break;
        case MSGPACK_OBJECT_FLOAT64:
          ret = mxCreateNumericMatrix(1, obj.via.array.size - nskip, mxDOUBLE_CLASS, mxREAL);
          ptrd = mxGetPr(ret);
          if (!any_nils) {
            // no nils, copy in vals
            for (size_t i = 0; i < obj.via.array.size; i++)
              ptrd[i] = obj.via.array.ptr[i].via.f64;
          } else {
            if (flags.unpack_nil_array_skip) {
              // Some nils, skip them
              size_t ptr_i = 0;
              for (size_t obj_i = 0; obj_i < obj.via.array.size; obj_i++) {
                if (nils[obj_i]) continue;
                else ptrd[ptr_i++] = obj.via.array.ptr[obj_i].via.f64; // incr ptr_i after use
              }
            } else {
              // some nils, set them to zero or NaN.
              double nil_val = (flags.unpack_nil == UNPACK_NIL_NAN) ? mxGetNaN() : 0;
              for (size_t i = 0; i < obj.via.array.size; i++) {
                if (nils[i]) ptrd[i] = nil_val;
                else ptrd[i] = obj.via.array.ptr[i].via.f64;
              }
            }
          }
          break;
        default:
         mexErrMsgIdAndTxt("msgpack:invalid_object_type",
                           "Shouldn't get here. Object type is %d", unique_scalar_type);
      }
    }
  } else {
    // Unpack to cell array
    ret = mxCreateCellMatrix(1, obj.via.array.size);
    for (size_t i = 0; i < obj.via.array.size; i++) {
      msgpack_object ob = obj.via.array.ptr[i];
      mxSetCell(ret, i, unpack_obj(ob));
    }
  }
  return ret;
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

  plhs[0] = unpack_obj(obj);
}

void pack_mxArray(msgpack_packer *pk, int nrhs, const mxArray* prhs) {
  unsigned int classid = mxGetClassID(prhs);
  if (classid > 0 && classid < 16 && classid != 5) {
    (*PackMap[classid])(pk, nrhs, prhs);
  } else {
    // 0 is UNKNOWN, 5 is VOID, 16-18 are FUNCTION, OPAQUE, & OBJECT
    const char* classname = mxGetClassName(prhs);
    if (flags.pack_other_as_nil) {
      mexWarnMsgIdAndTxt("msgpack:pack_other_as_nil",
                         "Packing class id %u (%s) as nil", classid, classname);
    } else {
      mexErrMsgIdAndTxt("msgpack:no_packing_method",
                        "No method for packing class id: %u, name: %s", classid, classname);
    }
  }
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
    pack_mxArray(pk, nrhs, pm);
  }
}

void mex_pack_struct(msgpack_packer *pk, int nrhs, const mxArray *prhs) {
  int nField = mxGetNumberOfFields(prhs);
  if (nField > 1) msgpack_pack_map(pk, nField);
  const char* field_name = NULL;
  int fieldname_len = 0;
  int ifield = 0;
  for (int i = 0; i < nField; i++) {
    field_name = mxGetFieldNameByNumber(prhs, i);
    fieldname_len = strlen(field_name);
    msgpack_pack_str(pk, fieldname_len);
    msgpack_pack_str_body(pk, field_name, fieldname_len);
    ifield = mxGetFieldNumber(prhs, field_name);
    mxArray* pm = mxGetFieldByNumber(prhs, 0, ifield);
    pack_mxArray(pk, nrhs, pm);
  }
}

void mex_pack(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  /* creates buffer and serializer instance. */
  msgpack_sbuffer* buffer = msgpack_sbuffer_new();
  msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  for (int i = 0; i < nrhs; i ++)
      pack_mxArray(pk, nrhs, prhs[i]);

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

// mex_unpacker utility functions
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
      ret = mxArrayRes_new(ret, unpack_obj(obj));
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
      cells.push_back(unpack_obj(obj));
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

    // These pack to MessagePack types. Others raise errors or pack to nil.
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
    else if (*it == "+pack_other_as_nil") flags.pack_other_as_nil = true;
    else if (*it == "-pack_other_as_nil") flags.pack_other_as_nil = false;
    else if (it->length() > 12 && it->substr(1, 11) == "unpack_nil_") {
      string remainder = it->substr(12, it->length() - 12);
      if (remainder == "zero") flags.unpack_nil = UNPACK_NIL_ZERO;
      else if (remainder == "NaN") flags.unpack_nil = UNPACK_NIL_NAN;
      else if (remainder == "empty") flags.unpack_nil = UNPACK_NIL_EMPTY;
      else if (remainder == "cell") flags.unpack_nil = UNPACK_NIL_CELL;
      else if (*it == "+unpack_nil_array_skip") flags.unpack_nil_array_skip = true;
      else if (*it == "-unpack_nil_array_skip") flags.unpack_nil_array_skip = false;
    }
    else mexErrMsgIdAndTxt("msgpack:invalid_flag", "%s is not a valid flag.", it->c_str());
  }
  // Handle command
  if (cmd == "set_flags") {
    // flags already processed above
    return;
  } else if (cmd == "reset_flags") {
    flags = mp_flags();
    return;
  } else if (cmd == "print_flags") {
    print_flags();
    return;
  } else if (cmd == "pack")
    mex_pack(nlhs, plhs, nrhs-1, prhs+1);
  else if (cmd == "unpack")
    mex_unpack(nlhs, plhs, nrhs-1, prhs+1);
  else if (cmd == "unpacker")
    mex_unpacker_std(nlhs, plhs, nrhs-1, prhs+1);
  else if (cmd == "help")
    mexPrintf(
      "See README.md for full details.\n"
      "The following flags may be set (+flag) or unset (-flag) in the command string.\n"
      "  pack_u8_bin\n"
      "  unicode_strs\n"
      "  unpack_map_as_cells\n"
      "  unpack_narrow\n"
      "  unpack_ext_w_tag\n"
      "  pack_other_as_nil\n"
      "  unpack_nil_array_skip\n"
      "Also, +unpack_nil_ may be set as one of the following (no unset):\n"
      "  +unpack_nil_zero (default)\n"
      "  +unpack_nil_NaN\n"
      "  +unpack_nil_empty\n"
      "  +unpack_nil_cell\n"
      "\n");
  else
    mexErrMsgIdAndTxt("msgpack:bad_command",
                      "Unrecognized command %s. Try msgpack('help');", cmd.c_str());
}
