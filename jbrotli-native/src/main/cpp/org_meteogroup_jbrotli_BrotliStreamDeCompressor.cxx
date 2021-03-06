/*
 * Copyright (c) 2015 MeteoGroup Deutschland GmbH
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
 */

#include <jni.h>
#include <stdlib.h>
#include <string.h>

#include "../../../../brotli/dec/decode.h"
#include "./type_converters.h"
#include "./org_meteogroup_jbrotli_BrotliError.h"


#ifdef __cplusplus
extern "C" {
#endif

static jint getErrorCode(BrotliResult brotliResult) {
  switch (brotliResult) {
    case BROTLI_RESULT_ERROR:
      return org_meteogroup_jbrotli_BrotliError_DECOMPRESS_BROTLI_RESULT_ERROR;
    case BROTLI_RESULT_NEEDS_MORE_INPUT:
      return org_meteogroup_jbrotli_BrotliError_DECOMPRESS_BROTLI_RESULT_NEEDS_MORE_INPUT;
    case BROTLI_RESULT_NEEDS_MORE_OUTPUT:
      return org_meteogroup_jbrotli_BrotliError_DECOMPRESS_BROTLI_RESULT_NEEDS_MORE_OUTPUT;
    case BROTLI_RESULT_SUCCESS:
      // fall through
    default:
      return 0;
  }
}

static jobject newResult(JNIEnv *env,
                         jint errorCode,
                         jint bytesConsumed,
                         jint bytesProduced) {
  jclass resultClass = env->FindClass("org/meteogroup/jbrotli/NativeDeCompressorResult");
  if (resultClass == NULL) {
    return NULL;
  }
  jmethodID methodID = env->GetMethodID(resultClass, "<init>", "(III)V");
  if (methodID == NULL) {
    return NULL;
  }
  return env->NewObject(resultClass, methodID, errorCode, bytesConsumed, bytesProduced);
}

static jobject newErrorResult(JNIEnv *env,
                              jint errorCode) {
  return newResult(env, errorCode, 0, 0);
}

static jfieldID brotliDeCompressorStateRefId;

/*
 * Class:     org_meteogroup_jbrotli_BrotliStreamDeCompressor
 * Method:    initJavaFieldIdCache
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_meteogroup_jbrotli_BrotliStreamDeCompressor_initJavaFieldIdCache(JNIEnv *env,
                                                                                              jclass cls) {
  brotliDeCompressorStateRefId = env->GetFieldID(cls, "brotliDeCompressorState", "J");
  if (NULL == brotliDeCompressorStateRefId) {
    return org_meteogroup_jbrotli_BrotliError_NATIVE_GET_FIELD_ID_ERROR;
  }
  return 0;
}

/*
 * Class:     org_meteogroup_jbrotli_BrotliStreamDeCompressor
 * Method:    initBrotliDeCompressor
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_meteogroup_jbrotli_BrotliStreamDeCompressor_initBrotliDeCompressor(JNIEnv *env,
                                                                                                jobject thisObj) {
  BrotliState *brotliState = (BrotliState*) GetLongFieldAsPointer(env, thisObj, brotliDeCompressorStateRefId);
  if (NULL != brotliState) {
    BrotliDestroyState(brotliState);
  }
  brotliState = BrotliCreateState(0, 0, NULL);
  SetLongFieldFromPointer(env, thisObj, brotliDeCompressorStateRefId, brotliState);
  return 0;
}

/*
 * Class:     org_meteogroup_jbrotli_BrotliStreamDeCompressor
 * Method:    freeNativeResources
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_meteogroup_jbrotli_BrotliStreamDeCompressor_freeNativeResources(JNIEnv *env,
                                                                                             jobject thisObj) {
  BrotliState *brotliState = (BrotliState*) GetLongFieldAsPointer(env, thisObj, brotliDeCompressorStateRefId);
  if (NULL != brotliState) {
    BrotliDestroyState(brotliState);
    brotliState = NULL;
    SetLongFieldFromPointer(env, thisObj, brotliDeCompressorStateRefId, brotliState);
  }
  return 0;
}

/*
 * Class:     org_meteogroup_jbrotli_BrotliStreamDeCompressor
 * Method:    deCompressBytes
 * Signature: ([BII[BII)Lorg/meteogroup/jbrotli/NativeDeCompressorResult;
 */
JNIEXPORT jobject JNICALL Java_org_meteogroup_jbrotli_BrotliStreamDeCompressor_deCompressBytes(JNIEnv *env,
                                                                                             jobject thisObj,
                                                                                             jbyteArray inByteArray,
                                                                                             jint inPosition,
                                                                                             jint inLength,
                                                                                             jbyteArray outByteArray,
                                                                                             jint outPosition,
                                                                                             jint outLength) {

  if (inPosition < 0 || inLength < 0 ) {
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "Brotli: input array position and length must be greater than zero.");
    return NULL;
  }
  if (outPosition < 0 || outLength < 0 ) {
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "Brotli: output array position and length must be greater than zero.");
    return NULL;
  }

  BrotliState *brotliState = (BrotliState*) GetLongFieldAsPointer(env, thisObj, brotliDeCompressorStateRefId);
  if (NULL == brotliState || env->ExceptionCheck()) {
    env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "BrotliStreamDeCompressor wasn't initialised. You need to create a new object before start decompressing.");
    return NULL;
  }

  uint8_t *inBufPtr = (uint8_t *) env->GetPrimitiveArrayCritical(inByteArray, 0);
  if (inBufPtr == NULL || env->ExceptionCheck()) return newErrorResult(env, org_meteogroup_jbrotli_BrotliError_DECOMPRESS_GetPrimitiveArrayCritical_INBUF);
  uint8_t *outBufPtr = (uint8_t *) env->GetPrimitiveArrayCritical(outByteArray, 0);
  if (outBufPtr == NULL || env->ExceptionCheck()) return newErrorResult(env, org_meteogroup_jbrotli_BrotliError_DECOMPRESS_GetPrimitiveArrayCritical_OUTBUF);

  size_t available_in = inLength;
  const uint8_t* inBufPtrInclPosition = inBufPtr + inPosition;
  uint8_t* outBufPtrInclPosition = outBufPtr + outPosition;
  size_t available_out = outLength;
  size_t total_out = 0;
  BrotliResult brotliResult = BrotliDecompressStream(&available_in, &inBufPtrInclPosition, &available_out, &outBufPtrInclPosition, &total_out, brotliState);

  env->ReleasePrimitiveArrayCritical(outByteArray, outBufPtr, 0);
  if (env->ExceptionCheck()) return newErrorResult(env, org_meteogroup_jbrotli_BrotliError_DECOMPRESS_ReleasePrimitiveArrayCritical_OUTBUF);
  env->ReleasePrimitiveArrayCritical(inByteArray, inBufPtr, 0);
  if (env->ExceptionCheck()) return newErrorResult(env, org_meteogroup_jbrotli_BrotliError_DECOMPRESS_ReleasePrimitiveArrayCritical_INBUF);

  jint errorCode = getErrorCode(brotliResult);
  jint bytesConsumed = (jint) inLength - available_in;
  jint bytesProduced = (jint) outLength - available_out;

  return newResult(env, errorCode, bytesConsumed, bytesProduced);
}

/*
 * Class:     org_meteogroup_jbrotli_BrotliStreamDeCompressor
 * Method:    deCompressByteBuffer
 * Signature: (Ljava/nio/ByteBuffer;IILjava/nio/ByteBuffer;II)Lorg/meteogroup/jbrotli/NativeDeCompressorResult;
 */
JNIEXPORT jobject JNICALL Java_org_meteogroup_jbrotli_BrotliStreamDeCompressor_deCompressByteBuffer(JNIEnv *env,
                                                                                                  jobject thisObj,
                                                                                                  jobject inBuf,
                                                                                                  jint inPosition,
                                                                                                  jint inLength,
                                                                                                  jobject outBuf,
                                                                                                  jint outPosition,
                                                                                                  jint outLength) {

  if (inPosition < 0 || inLength < 0 ) {
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "Brotli: input ByteBuffer position and length must be greater than zero.");
    return NULL;
  }
  if (outPosition < 0 || outLength < 0 ) {
    env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"), "Brotli: output ByteBuffer position and length must be greater than zero.");
    return NULL;
  }

  BrotliState *brotliState = (BrotliState*) GetLongFieldAsPointer(env, thisObj, brotliDeCompressorStateRefId);
  if (NULL == brotliState || env->ExceptionCheck()) {
    env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "BrotliStreamDeCompressor wasn't initialised. You need to create a new object before start decompressing.");
    return NULL;
  }

  const uint8_t *inBufPtr = (uint8_t *) env->GetDirectBufferAddress(inBuf);
  if (NULL == inBufPtr) {
    env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "BrotliStreamDeCompressor couldn't get direct address of input buffer.");
    return NULL;
  }
  uint8_t *outBufPtr = (uint8_t *) env->GetDirectBufferAddress(outBuf);
  if (NULL == outBufPtr) {
    env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "BrotliStreamDeCompressor couldn't get direct address of output buffer.");
    return NULL;
  }

  size_t available_in = inLength;
  inBufPtr += inPosition;
  size_t available_out = outLength;
  outBufPtr += outPosition;
  size_t total_out = 0;
  BrotliResult brotliResult = BrotliDecompressStream(&available_in, &inBufPtr, &available_out, &outBufPtr, &total_out, brotliState);

  jint errorCode = getErrorCode(brotliResult);
  jint bytesConsumed = (jint) inLength - available_in;
  jint bytesProduced = (jint) outLength - available_out;

  return newResult(env, errorCode, bytesConsumed, bytesProduced);
}

#ifdef __cplusplus
}
#endif

