//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information. 
//

#pragma once

#if !defined(_WIN32) && !defined(__STDC_LIB_EXT1__)

#ifndef _ERRNO_T_DEFINED
#define _ERRNO_T_DEFINED
typedef int errno_t;
#endif

#ifndef _VALIDATE_RETURN_ERRCODE
#define _VALIDATE_RETURN_ERRCODE(c, e) \
    if (!(c)) return e
#endif

errno_t /*__cdecl*/ memcpy_s(
    void * dst,
    size_t sizeInBytes,
    const void * src,
    size_t count
)
{
    if (count == 0)
    {
        /* nothing to do */
        return 0;
    }

    /* validation section */
    _VALIDATE_RETURN_ERRCODE(dst != NULL, EINVAL);
    if (src == NULL || sizeInBytes < count)
    {
        /* zeroes the destination buffer */
        memset(dst, 0, sizeInBytes);

        _VALIDATE_RETURN_ERRCODE(src != NULL, EINVAL);
        _VALIDATE_RETURN_ERRCODE(sizeInBytes >= count, ERANGE);
        /* useless, but prefast is confused */
        return EINVAL;
    }

    memcpy(dst, src, count);
    return 0;
}

template<size_t sizeInBytes>
errno_t /*__cdecl*/ memcpy_s(
    const char (&dst)[sizeInBytes],
    const void * src,
    size_t count
) {
    return memcpy_s(dst, sizeInBytes, src, count);
}
#endif
