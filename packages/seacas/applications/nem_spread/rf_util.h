/*
 * Copyright (C) 2009 Sandia Corporation.  Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
 * certain rights in this software
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 * 
 *     * Neither the name of Sandia Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
#ifndef _RF_UTIL_H
#define _RF_UTIL_H
#include <limits.h>

template <typename T>
inline void my_swap(T &r, T &s)
{
  T tmp = r;
  r = s;
  s = tmp;
}

template <typename T, typename U>
void siftDown( T *a, U *b, int64_t start, int64_t end)
{
  int64_t root = start;
 
  while ( root*2+1 < end ) {
    int64_t child = 2*root + 1;
    if ((child + 1 < end) && (a[child] < a[child+1])) {
      child += 1;
    }
    if (a[root] < a[child]) {
      my_swap(a[child], a[root] );
      my_swap(b[child], b[root] );
      root = child;
    }
    else
      return;
  }
}

template <typename T, typename U>
void my_sort(int64_t count, T ra[], U rb[])
{
  int64_t start, end;
 
  /* heapify */
  for (start = (count-2)/2; start >=0; start--) {
    siftDown( ra, rb, start, count);
  }
 
  for (end=count-1; end > 0; end--) {
    my_swap(ra[end],ra[0]);
    my_swap(rb[end],rb[0]);
    siftDown(ra, rb, 0, end);
  }
}
 
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
extern void  print_line     (const char *charstr, int ntimes);
extern int   break_message_up(size_t, size_t, size_t, int **);

#endif /* #ifndef _RF_UTIL_H */
