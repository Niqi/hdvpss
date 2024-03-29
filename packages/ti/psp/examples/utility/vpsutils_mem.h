/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/

/**
    \ingroup VPSUTILS_API
    \defgroup VPSUTILS_MEM_API Frame buffer memory allocator API

    APIs to allocate frame buffer memory from a predefined memory pool

    @{
*/

/**
  \file vpsutils_mem.h
  \brief Frame buffer memory allocator API
*/


#ifndef _VPSUTILS_MEM_H_
#define _VPSUTILS_MEM_H_

#include <ti/sysbios/BIOS.h>
#include <ti/psp/vps/fvid2.h>

/** \brief Utility define for Kilobyte, i.e 1024 bytes */
#ifndef KB
#define KB ((UInt32)1024)
#endif

/** \brief Utility define for Megabyte, i.e 1024*1024 bytes */
#ifndef MB
#define MB (KB*KB)
#endif


/**
  \brief One time system init of memory allocator

  Should be called by application before using allocate APIs

  \return 0 on sucess, else failure
*/
Int32 VpsUtils_memInit();

/**
  \brief One time system init of memory allocator

  Should be called by application before using allocate APIs.
  This is same as VpsUtils_memInit(), except that a larger heap size is used.
  Right now only chains app will used this large heap.
  When using large heap a different config.bld needs to be used.
  For chains app this is setup via Makefile's in HDVPSS driver build system.

  Only ONE of VpsUtils_memInit() or VpsUtils_memInit_largeHeap() should be called during system init.

  \return 0 on sucess, else failure
*/
Int32 VpsUtils_memInit_largeHeap();

/**
  \brief One time system de-init of memory allocator

  Should be called by application at system de-init

  \return 0 on sucess, else failure
*/
Int32 VpsUtils_memDeInit();

/**
  \brief Allocate a frame

  Use FVID2_Format to allocate a frame.
  Fill FVID2_Frame fields like channelNum based on FVID2_Format

  \param  pFormat   [IN] Data format information
  \param  pFrame    [OUT] Initialzed FVID2_Frame structure
  \param  numFrames [IN] Number of frames to allocate

  \return 0 on sucess, else failure
*/
Int32 VpsUtils_memFrameAlloc(FVID2_Format *pFormat,
        FVID2_Frame *pFrame, UInt16 numFrames);

/**
  \brief Free's previously allocate FVID2_Frame's

  \param  pFormat   [IN] Data format information
  \param  pFrame    [IN] FVID2_Frame structure
  \param  numFrames [IN] Number of frames to free

  \return 0 on sucess, else failure
*/
Int32 VpsUtils_memFrameFree(FVID2_Format *pFormat,
        FVID2_Frame *pFrame, UInt16 numFrames);

/**
  \brief Allocate memory from memory pool

  \param size   [IN] size in bytes
  \param align  [IN] alignment in bytes

  \return NULL or error, else memory pointer
*/
Ptr   VpsUtils_memAlloc(UInt32 size, UInt32 align);

/**
  \brief Free previously allocate memory pointer

  \param addr [IN] memory pointer to free
  \param size [IN] size of memory pointed to by the memory pointer

  \return 0 on sucess, else failure
*/
Int32 VpsUtils_memFree(Ptr addr, UInt32 size);

/**
    \brief Control if allocated buffer needs to be cleared to 0

    By default allocated buffer will not be cleared to 0

    \param enable   [IN] TRUE: clear allocated buffer, FALSE: do not clear allocated buffer

    \return 0 on sucess, else failure
*/
Int32 VpsUtils_memClearOnAlloc(Bool enable);

/**
 *  \brief Returns the system heap free memory (in bytes)
 */
UInt32 VpsUtils_memGetSystemHeapFreeSpace(void);

/**
 *  \brief Returns the buffer heap free memory (in bytes)
 */
UInt32 VpsUtils_memGetBufferHeapFreeSpace(void);


/**
 *  \brief Init heap with user specified memory area
 *
 *  This API is used internally by VpsUtils_memInit() and VpsUtils_memInit_largeHeap()
 *  so typically user's will NOT use this API directly
*/
Int32 VpsUtils_memInit_internal(UInt32 *pMemAddr, UInt32 memSize);

#endif


/*@}*/
