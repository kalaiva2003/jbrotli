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

package com.meteogroup.jbrotli;

import org.scijava.nativelib.NativeLoader;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.Test;

import java.nio.ByteBuffer;

import static com.meteogroup.jbrotli.BrotliCompressorTest.A_BYTES;
import static com.meteogroup.jbrotli.BrotliCompressorTest.A_BYTES_COMPRESSED;
import static com.meteogroup.jbrotli.BufferTestHelper.*;
import static java.nio.ByteBuffer.wrap;
import static org.assertj.core.api.Assertions.assertThat;

public class BrotliStreamCompressorByteBufferTest {

  private BrotliStreamCompressor compressor;

  @BeforeClass
  public void loadLibrary() throws Exception {
    NativeLoader.loadLibrary("brotli");
  }

  @BeforeMethod
  public void setUp() throws Exception {
    compressor = new BrotliStreamCompressor(Brotli.DEFAULT_PARAMETER);
  }

  @AfterMethod
  public void tearDown() throws Exception {
    compressor.close();
  }

  @Test
  public void compress_sets_position_to_the_next_compressed_block__until_hits_limit() throws Exception {
    // setup
    int maxInputBufferSize = compressor.getMaxInputBufferSize();
    int expectedLargeBufferSize = (int) (maxInputBufferSize * 2.5);

    // given
    ByteBuffer in = wrapDirect(createFilledByteArray(expectedLargeBufferSize, 'x'));

    // when #1
    compressor.compressNextBuffer(in, false);
    // then
    assertThat(in.position()).isEqualTo(maxInputBufferSize);
    assertThat(in.limit()).isEqualTo(expectedLargeBufferSize);
    assertThat(in.capacity()).isEqualTo(expectedLargeBufferSize);

    // when #2
    compressor.compressNextBuffer(in, false);
    // then
    assertThat(in.position()).isEqualTo(maxInputBufferSize * 2);
    assertThat(in.limit()).isEqualTo(expectedLargeBufferSize);
    assertThat(in.capacity()).isEqualTo(expectedLargeBufferSize);

    // when #2.5
    compressor.compressNextBuffer(in, false);
    // then
    assertThat(in.position()).isEqualTo(expectedLargeBufferSize);
    assertThat(in.limit()).isEqualTo(expectedLargeBufferSize);
    assertThat(in.capacity()).isEqualTo(expectedLargeBufferSize);
  }


  //
  // *** direct ByteBuffer **********

  @Test
  public void compress_with_direct_ByteBuffer_and_flushing() throws Exception {
    ByteBuffer inBuffer = ByteBuffer.allocateDirect(A_BYTES.length);
    inBuffer.put(A_BYTES);
    inBuffer.position(0);

    // when
    ByteBuffer outBuffer = compressor.compressNextBuffer(inBuffer, true);

    assertThat(outBuffer.capacity()).isEqualTo(A_BYTES_COMPRESSED.length);
    assertThat(getByteArray(outBuffer)).startsWith(A_BYTES_COMPRESSED);
  }

  @Test
  public void compress_with_direct_ByteBuffer_without_flushing() throws Exception {
    ByteBuffer inBuffer = ByteBuffer.allocateDirect(A_BYTES.length);
    inBuffer.put(A_BYTES);
    inBuffer.position(0);

    // when
    ByteBuffer outBuffer = compressor.compressNextBuffer(inBuffer, false);
    // then
    assertThat(outBuffer.capacity()).isEqualTo(0);

    // when
    outBuffer = compressor.compressNextBuffer(ByteBuffer.allocateDirect(0), true);
    // then
    assertThat(outBuffer.capacity()).isEqualTo(A_BYTES_COMPRESSED.length);

    // then
    assertThat(getByteArray(outBuffer)).startsWith(A_BYTES_COMPRESSED);
  }

  @Test
  public void compress_with_direct_ByteBuffer_using_position_and_length() throws Exception {
    // setup
    ByteBuffer inBuffer = ByteBuffer.allocateDirect(100);
    inBuffer.put(createFilledByteArray(100, 'x'));

    // given
    int testPosition = 23;
    inBuffer.position(testPosition);
    inBuffer.put(A_BYTES);
    inBuffer.position(testPosition);
    inBuffer.limit(testPosition + A_BYTES.length);

    // when
    ByteBuffer outBuffer = compressor.compressNextBuffer(inBuffer, true);

    // then
    assertThat(outBuffer.capacity()).isEqualTo(A_BYTES_COMPRESSED.length);
    assertThat(outBuffer.limit()).isEqualTo(A_BYTES_COMPRESSED.length);
    assertThat(outBuffer.position()).isEqualTo(0);
    // then
    assertThat(getByteArray(outBuffer)).startsWith(A_BYTES_COMPRESSED);
  }

  //
  // *** array wrapped ByteBuffer **********

  @Test
  public void compress_with_array_wrapped_ByteBuffer_and_flushing() throws Exception {
    ByteBuffer inBuffer = wrap(A_BYTES);

    // when
    ByteBuffer outBuffer = compressor.compressNextBuffer(inBuffer, true);

    assertThat(outBuffer.capacity()).isEqualTo(A_BYTES_COMPRESSED.length);
    assertThat(getByteArray(outBuffer)).isEqualTo(A_BYTES_COMPRESSED);
  }

  @Test
  public void compress_with_array_wrapped_ByteBuffer_without_flushing() throws Exception {
    ByteBuffer inBuffer = wrap(A_BYTES);

    // when
    ByteBuffer outBuffer = compressor.compressNextBuffer(inBuffer, false);
    // then
    assertThat(outBuffer.capacity()).isEqualTo(0);

    // when
    outBuffer = compressor.compressNextBuffer(wrap(new byte[0]), true);
    // then
    assertThat(outBuffer.capacity()).isEqualTo(A_BYTES_COMPRESSED.length);

    // then
    assertThat(getByteArray(outBuffer)).startsWith(A_BYTES_COMPRESSED);
  }

  @Test
  public void compress_with_array_wrapped_ByteBuffer_using_position_and_length() throws Exception {
    // setup
    ByteBuffer inBuffer = wrap(createFilledByteArray(100, 'x'));

    // given
    int testPosition = 23;
    inBuffer.position(testPosition);
    inBuffer.put(A_BYTES);
    inBuffer.position(testPosition);
    inBuffer.limit(testPosition + A_BYTES.length);

    // when
    ByteBuffer outBuffer = compressor.compressNextBuffer(inBuffer, true);

    // then
    assertThat(outBuffer.capacity()).isEqualTo(A_BYTES_COMPRESSED.length);
    assertThat(outBuffer.limit()).isEqualTo(A_BYTES_COMPRESSED.length);
    assertThat(outBuffer.position()).isEqualTo(0);
    // then
    assertThat(getByteArray(outBuffer)).startsWith(A_BYTES_COMPRESSED);
  }

  @Test
  public void compress_with_array_wrapped_ByteBuffer_using_arrayOffset_and_length() throws Exception {
    // setup
    ByteBuffer inBuffer = wrap(createFilledByteArray(100, 'x'));

    // given
    int testPosition = 23;
    inBuffer.position(testPosition);
    inBuffer.put(A_BYTES);
    inBuffer.position(testPosition);
    inBuffer = inBuffer.slice();
    inBuffer.limit(A_BYTES.length);

    // when
    ByteBuffer outBuffer = compressor.compressNextBuffer(inBuffer, true);

    // then
    assertThat(outBuffer.capacity()).isEqualTo(A_BYTES_COMPRESSED.length);
    assertThat(outBuffer.limit()).isEqualTo(A_BYTES_COMPRESSED.length);
    assertThat(outBuffer.position()).isEqualTo(0);
    // then
    assertThat(getByteArray(outBuffer)).startsWith(A_BYTES_COMPRESSED);
  }

}