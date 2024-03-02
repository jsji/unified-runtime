//===--------- image.cpp - Level Zero Adapter -----------------------------===//
//
// Copyright (C) 2023 Intel Corporation
//
// Part of the Unified-Runtime Project, under the Apache License v2.0 with LLVM
// Exceptions. See LICENSE.TXT
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "image.hpp"
#include "common.hpp"
#include "context.hpp"

static ur_result_t getImageRegionHelper(ze_image_desc_t ZeImageDesc,
                                        ur_rect_offset_t *Origin,
                                        ur_rect_region_t *Region,
                                        ze_image_region_t &ZeRegion) {
  UR_ASSERT(Origin, UR_RESULT_ERROR_INVALID_VALUE);
  UR_ASSERT(Region, UR_RESULT_ERROR_INVALID_VALUE);

  if (ZeImageDesc.type == ZE_IMAGE_TYPE_1D) {
    Region->height = 1;
    Region->depth = 1;
  } else if (ZeImageDesc.type == ZE_IMAGE_TYPE_2D || ZeImageDesc.type == ZE_IMAGE_TYPE_1DARRAY) {
    Region->depth = 1;
  }

#ifndef NDEBUG
  UR_ASSERT((ZeImageDesc.type == ZE_IMAGE_TYPE_1D && Origin->y == 0 &&
             Origin->z == 0) ||
                (ZeImageDesc.type == ZE_IMAGE_TYPE_1DARRAY && Origin->z == 0) ||
                (ZeImageDesc.type == ZE_IMAGE_TYPE_2D && Origin->z == 0) ||
                (ZeImageDesc.type == ZE_IMAGE_TYPE_3D),
            UR_RESULT_ERROR_INVALID_VALUE);

  UR_ASSERT(Region->width && Region->height && Region->depth,
            UR_RESULT_ERROR_INVALID_VALUE);
  UR_ASSERT(
      (ZeImageDesc.type == ZE_IMAGE_TYPE_1D && Region->height == 1 &&
       Region->depth == 1) ||
          (ZeImageDesc.type == ZE_IMAGE_TYPE_1DARRAY && Region->depth == 1) ||
          (ZeImageDesc.type == ZE_IMAGE_TYPE_2D && Region->depth == 1) ||
          (ZeImageDesc.type == ZE_IMAGE_TYPE_3D),
      UR_RESULT_ERROR_INVALID_VALUE);
#endif // !NDEBUG

  uint32_t OriginX = ur_cast<uint32_t>(Origin->x);
  uint32_t OriginY = ur_cast<uint32_t>(Origin->y);
  uint32_t OriginZ = ur_cast<uint32_t>(Origin->z);

  uint32_t Width = ur_cast<uint32_t>(Region->width);
  uint32_t Height = ur_cast<uint32_t>(Region->height);
  uint32_t Depth = ur_cast<uint32_t>(Region->depth);

  ZeRegion = {OriginX, OriginY, OriginZ, Width, Height, Depth};

  return UR_RESULT_SUCCESS;
}

ur_result_t ur2zeImageDesc(const ur_image_format_t *ImageFormat,
                           const ur_image_desc_t *ImageDesc,
                           ZeStruct<ze_image_desc_t> &ZeImageDesc) {

  ze_image_format_type_t ZeImageFormatType;
  size_t ZeImageFormatTypeSize;
  switch (ImageFormat->channelType) {
  case UR_IMAGE_CHANNEL_TYPE_FLOAT: {
    ZeImageFormatType = ZE_IMAGE_FORMAT_TYPE_FLOAT;
    ZeImageFormatTypeSize = 32;
    break;
  }
  case UR_IMAGE_CHANNEL_TYPE_HALF_FLOAT: {
    ZeImageFormatType = ZE_IMAGE_FORMAT_TYPE_FLOAT;
    ZeImageFormatTypeSize = 16;
    break;
  }
  case UR_IMAGE_CHANNEL_TYPE_UNSIGNED_INT32: {
    ZeImageFormatType = ZE_IMAGE_FORMAT_TYPE_UINT;
    ZeImageFormatTypeSize = 32;
    break;
  }
  case UR_IMAGE_CHANNEL_TYPE_UNSIGNED_INT16: {
    ZeImageFormatType = ZE_IMAGE_FORMAT_TYPE_UINT;
    ZeImageFormatTypeSize = 16;
    break;
  }
  case UR_IMAGE_CHANNEL_TYPE_UNSIGNED_INT8: {
    ZeImageFormatType = ZE_IMAGE_FORMAT_TYPE_UINT;
    ZeImageFormatTypeSize = 8;
    break;
  }
  case UR_IMAGE_CHANNEL_TYPE_UNORM_INT16: {
    ZeImageFormatType = ZE_IMAGE_FORMAT_TYPE_UNORM;
    ZeImageFormatTypeSize = 16;
    break;
  }
  case UR_IMAGE_CHANNEL_TYPE_UNORM_INT8: {
    ZeImageFormatType = ZE_IMAGE_FORMAT_TYPE_UNORM;
    ZeImageFormatTypeSize = 8;
    break;
  }
  case UR_IMAGE_CHANNEL_TYPE_SIGNED_INT32: {
    ZeImageFormatType = ZE_IMAGE_FORMAT_TYPE_SINT;
    ZeImageFormatTypeSize = 32;
    break;
  }
  case UR_IMAGE_CHANNEL_TYPE_SIGNED_INT16: {
    ZeImageFormatType = ZE_IMAGE_FORMAT_TYPE_SINT;
    ZeImageFormatTypeSize = 16;
    break;
  }
  case UR_IMAGE_CHANNEL_TYPE_SIGNED_INT8: {
    ZeImageFormatType = ZE_IMAGE_FORMAT_TYPE_SINT;
    ZeImageFormatTypeSize = 8;
    break;
  }
  case UR_IMAGE_CHANNEL_TYPE_SNORM_INT16: {
    ZeImageFormatType = ZE_IMAGE_FORMAT_TYPE_SNORM;
    ZeImageFormatTypeSize = 16;
    break;
  }
  case UR_IMAGE_CHANNEL_TYPE_SNORM_INT8: {
    ZeImageFormatType = ZE_IMAGE_FORMAT_TYPE_SNORM;
    ZeImageFormatTypeSize = 8;
    break;
  }
  default:
    urPrint("urMemImageCreate: unsupported image data type: data type = %d\n",
            ImageFormat->channelType);
    return UR_RESULT_ERROR_INVALID_VALUE;
  }

  // TODO: populate the layout mapping
  ze_image_format_layout_t ZeImageFormatLayout;
  switch (ImageFormat->channelOrder) {
  case UR_IMAGE_CHANNEL_ORDER_A:
  case UR_IMAGE_CHANNEL_ORDER_R: {
    switch (ZeImageFormatTypeSize) {
    case 8:
      ZeImageFormatLayout = ZE_IMAGE_FORMAT_LAYOUT_8;
      break;
    case 16:
      ZeImageFormatLayout = ZE_IMAGE_FORMAT_LAYOUT_16;
      break;
    case 32:
      ZeImageFormatLayout = ZE_IMAGE_FORMAT_LAYOUT_32;
      break;
    default:
      urPrint("urMemImageCreate: unexpected data type Size\n");
      return UR_RESULT_ERROR_INVALID_VALUE;
    }
    break;
  }
  case UR_IMAGE_CHANNEL_ORDER_RG:
  case UR_IMAGE_CHANNEL_ORDER_RA:
  case UR_IMAGE_CHANNEL_ORDER_RX: {
    switch (ZeImageFormatTypeSize) {
    case 8:
      ZeImageFormatLayout = ZE_IMAGE_FORMAT_LAYOUT_8_8;
      break;
    case 16:
      ZeImageFormatLayout = ZE_IMAGE_FORMAT_LAYOUT_16_16;
      break;
    case 32:
      ZeImageFormatLayout = ZE_IMAGE_FORMAT_LAYOUT_32_32;
      break;
    default:
      urPrint("urMemImageCreate: unexpected data type Size\n");
      return UR_RESULT_ERROR_INVALID_VALUE;
    }
    break;
  }
  case UR_IMAGE_CHANNEL_ORDER_RGB:
  case UR_IMAGE_CHANNEL_ORDER_RGX: {
    switch (ZeImageFormatTypeSize) {
    default:
      urPrint("urMemImageCreate: unexpected data type Size\n");
      return UR_RESULT_ERROR_INVALID_VALUE;
    }
    break;
  }
  case UR_IMAGE_CHANNEL_ORDER_RGBX:
  case UR_IMAGE_CHANNEL_ORDER_RGBA:
  case UR_IMAGE_CHANNEL_ORDER_ARGB:
  case UR_IMAGE_CHANNEL_ORDER_BGRA: {
    switch (ZeImageFormatTypeSize) {
    case 8:
      ZeImageFormatLayout = ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8;
      break;
    case 16:
      ZeImageFormatLayout = ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16;
      break;
    case 32:
      ZeImageFormatLayout = ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32;
      break;
    default:
      urPrint("urMemImageCreate: unexpected data type Size\n");
      return UR_RESULT_ERROR_INVALID_VALUE;
    }
    break;
  }
  default:
    urPrint("format layout = %d\n", ImageFormat->channelOrder);
    die("urMemImageCreate: unsupported image format layout\n");
    break;
  }

  ze_image_format_t ZeFormatDesc = {
      ZeImageFormatLayout, ZeImageFormatType,
      // TODO: are swizzles deducted from image_format->image_channel_order?
      ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_G,
      ZE_IMAGE_FORMAT_SWIZZLE_B, ZE_IMAGE_FORMAT_SWIZZLE_A};

  ze_image_type_t ZeImageType;
  switch (ImageDesc->type) {
  case UR_MEM_TYPE_IMAGE1D:
    ZeImageType = ZE_IMAGE_TYPE_1D;
    break;
  case UR_MEM_TYPE_IMAGE2D:
    ZeImageType = ZE_IMAGE_TYPE_2D;
    break;
  case UR_MEM_TYPE_IMAGE3D:
    ZeImageType = ZE_IMAGE_TYPE_3D;
    break;
  case UR_MEM_TYPE_IMAGE1D_ARRAY:
    ZeImageType = ZE_IMAGE_TYPE_1DARRAY;
    break;
  case UR_MEM_TYPE_IMAGE2D_ARRAY:
    ZeImageType = ZE_IMAGE_TYPE_2DARRAY;
    break;
  default:
    urPrint("urMemImageCreate: unsupported image type\n");
    return UR_RESULT_ERROR_INVALID_VALUE;
  }

  ZeImageDesc.pNext = ImageDesc->pNext;
  ZeImageDesc.arraylevels = ZeImageDesc.flags = 0;
  ZeImageDesc.type = ZeImageType;
  ZeImageDesc.format = ZeFormatDesc;
  ZeImageDesc.width = ur_cast<uint64_t>(ImageDesc->width);
  ZeImageDesc.height = std::max(ur_cast<uint64_t>(ImageDesc->height), 1ul);
  ZeImageDesc.depth = std::max(ur_cast<uint64_t>(ImageDesc->depth), 1ul);
  ZeImageDesc.arraylevels = ur_cast<uint32_t>(ImageDesc->arraySize);
  ZeImageDesc.miplevels = ImageDesc->numMipLevel;

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urUSMPitchedAllocExp(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    const ur_usm_desc_t *pUSMDesc, ur_usm_pool_handle_t pool,
    size_t widthInBytes, size_t height, size_t elementSizeBytes, void **ppMem,
    size_t *pResultPitch) {
  std::ignore = hContext;
  std::ignore = hDevice;
  std::ignore = pUSMDesc;
  std::ignore = pool;
  std::ignore = widthInBytes;
  std::ignore = height;
  std::ignore = elementSizeBytes;
  std::ignore = ppMem;
  std::ignore = pResultPitch;
  urPrint("[UR][L0] %s function not implemented!\n", __FUNCTION__);
  return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

UR_APIEXPORT ur_result_t UR_APICALL
urBindlessImagesUnsampledImageHandleDestroyExp(ur_context_handle_t hContext,
                                               ur_device_handle_t hDevice,
                                               ur_exp_image_handle_t hImage) {
  std::ignore = hContext;
  std::ignore = hDevice;
#if 1
  auto ZeResult = ZE_CALL_NOCHECK(
      zeImageDestroy, (reinterpret_cast<ze_image_handle_t>(hImage)));
  // Gracefully handle the case that L0 was already unloaded.
  if (ZeResult && ZeResult != ZE_RESULT_ERROR_UNINITIALIZED)
    return ze2urResult(ZeResult);
#else
  char *ZeHandleImage;
  auto Image = (_ur_image *)hImage;
  if (Image->OwnNativeHandle) {
    UR_CALL(Image->getZeHandle(ZeHandleImage, ur_mem_handle_t_::write_only));
    auto ZeResult = ZE_CALL_NOCHECK(
        zeImageDestroy, (ur_cast<ze_image_handle_t>(ZeHandleImage)));
    // Gracefully handle the case that L0 was already unloaded.
    if (ZeResult && ZeResult != ZE_RESULT_ERROR_UNINITIALIZED)
      return ze2urResult(ZeResult);
  }

  delete Image;
#endif
  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL
urBindlessImagesSampledImageHandleDestroyExp(ur_context_handle_t hContext,
                                             ur_device_handle_t hDevice,
                                             ur_exp_image_handle_t hImage) {
  std::ignore = hContext;
  std::ignore = hDevice;
  printf("[urBindlessImagesSampledImageHandleDestroyExp] END, image: %p\n\n", (void*)hImage);

  auto ZeResult = ZE_CALL_NOCHECK(
      zeImageDestroy, (reinterpret_cast<ze_image_handle_t>(hImage)));
  // Gracefully handle the case that L0 was already unloaded.
  if (ZeResult && ZeResult != ZE_RESULT_ERROR_UNINITIALIZED)
    return ze2urResult(ZeResult);
  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesImageAllocateExp(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    const ur_image_format_t *pImageFormat, const ur_image_desc_t *pImageDesc,
    ur_exp_image_mem_handle_t *phImageMem) {
  std::shared_lock<ur_shared_mutex> Lock(hContext->Mutex);

  ZeStruct<ze_image_desc_t> ZeImageDesc;
  UR_CALL(ur2zeImageDesc(pImageFormat, pImageDesc, ZeImageDesc));
  ZeImageDesc.miplevels = 0;

  ze_image_mem_handle_exp_t ImageMem;
  ZE2UR_CALL(zeImageMemAllocExp,
             (hContext->ZeContext, hDevice->ZeDevice, &ZeImageDesc, &ImageMem));

  *phImageMem = reinterpret_cast<ur_exp_image_mem_handle_t>(ImageMem);
  printf("[urBindlessImagesImageAllocateExp] ImageMem %p\n", (void*)ImageMem);

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesImageFreeExp(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    ur_exp_image_mem_handle_t hImageMem) {
  std::ignore = hDevice;
  printf("[urBindlessImagesImageFreeExp] hImageMem %p\n", (void*)hImageMem);
  ZE2UR_CALL(zeImageMemFreeExp,
             (hContext->ZeContext, reinterpret_cast<ze_image_mem_handle_exp_t>(hImageMem)));
  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesUnsampledImageCreateExp(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    ur_exp_image_mem_handle_t hImageMem, const ur_image_format_t *pImageFormat,
    const ur_image_desc_t *pImageDesc, ur_mem_handle_t *phMem,
    ur_exp_image_handle_t *phImage) {
  std::shared_lock<ur_shared_mutex> Lock(hContext->Mutex);

  ZeStruct<ze_image_desc_t> ZeImageDesc;
  UR_CALL(ur2zeImageDesc(pImageFormat, pImageDesc, ZeImageDesc));
  ZeImageDesc.miplevels = 0;

  ze_image_mem_alloc_exp_desc_t imageAllocDesc = {
    ZE_STRUCTURE_TYPE_IMAGE_MEM_ALLOC_DESC, /* stype */
    nullptr,                                /* pNext */
    reinterpret_cast<ze_image_mem_handle_exp_t>(hImageMem) /* hImageMem */
  };
  ZeImageDesc.pNext = static_cast<void*>(&imageAllocDesc);

  ze_image_handle_t ZeImage;
  ZE2UR_CALL(zeImageCreate,
             (hContext->ZeContext, hDevice->ZeDevice, &ZeImageDesc, &ZeImage));
  ZE2UR_CALL(zeContextMakeImageResident,
             (hContext->ZeContext, hDevice->ZeDevice, ZeImage));
  try {
#if 1
    *phImage = (ur_exp_image_handle_t)ZeImage;
    printf("[urBindlessImagesUnsampledImageCreateExp] END, hImageMem %p, *phImage: %p\n\n", (void*)hImageMem, (void*)*phImage);
#else
    auto UrImage = new _ur_image(hContext, ZeImage, true /*OwnZeMemHandle*/);
    *phImage = (ur_exp_image_handle_t)UrImage;

#ifndef NDEBUG
    UrImage->ZeImageDesc = ZeImageDesc;
#endif // !NDEBUG
#endif
  } catch (const std::bad_alloc &) {
    return UR_RESULT_ERROR_OUT_OF_HOST_MEMORY;
  } catch (...) {
    return UR_RESULT_ERROR_UNKNOWN;
  }
  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesSampledImageCreateExp(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    ur_exp_image_mem_handle_t hImageMem, const ur_image_format_t *pImageFormat,
    const ur_image_desc_t *pImageDesc, ur_sampler_handle_t hSampler,
    ur_mem_handle_t *phMem, ur_exp_image_handle_t *phImage) {

  auto result = urBindlessImagesUnsampledImageCreateExp(hContext, hDevice, hImageMem, pImageFormat, pImageDesc, phMem, phImage);
  struct combined_sampled_image_handle {
    uint64_t raw_image_handle;
    uint64_t raw_sampler_handle;
  };
  combined_sampled_image_handle *sampledImageHandle = reinterpret_cast<combined_sampled_image_handle *>(phImage);
  sampledImageHandle->raw_image_handle = reinterpret_cast<uint64_t>(*phImage);
  sampledImageHandle->raw_sampler_handle = reinterpret_cast<uint64_t>(hSampler->ZeSampler);
  printf("[urBindlessImagesSampledImageCreateExp] END, image: 0x%lx, sampler 0x%lx\n\n", sampledImageHandle->raw_image_handle, sampledImageHandle->raw_sampler_handle);

  return result;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesImageCopyExp(
    ur_queue_handle_t hQueue, void *pDst, void *pSrc,
    const ur_image_format_t *pImageFormat, const ur_image_desc_t *pImageDesc,
    ur_exp_image_copy_flags_t imageCopyFlags, ur_rect_offset_t srcOffset,
    ur_rect_offset_t dstOffset, ur_rect_region_t copyExtent,
    ur_rect_region_t hostExtent, uint32_t numEventsInWaitList,
    const ur_event_handle_t *phEventWaitList, ur_event_handle_t *phEvent) {
  std::scoped_lock<ur_shared_mutex> Lock(hQueue->Mutex);
  ZeStruct<ze_image_desc_t> ZeImageDesc;
  UR_CALL(ur2zeImageDesc(pImageFormat, pImageDesc, ZeImageDesc));
  ZeImageDesc.miplevels = 0;

  bool UseCopyEngine = hQueue->useCopyEngine(/*PreferCopyEngine*/ true);

  _ur_ze_event_list_t TmpWaitList;
  UR_CALL(TmpWaitList.createAndRetainUrZeEventList(
      numEventsInWaitList, phEventWaitList, hQueue, UseCopyEngine));

  // We want to batch these commands to avoid extra submissions (costly)
  bool OkToBatch = true;

  // Get a new command list to be used on this call
  ur_command_list_ptr_t CommandList{};
  UR_CALL(hQueue->Context->getAvailableCommandList(hQueue, CommandList,
                                                   UseCopyEngine, OkToBatch));

  ze_event_handle_t ZeEvent = nullptr;
  ur_event_handle_t InternalEvent;
  bool IsInternal = phEvent == nullptr;
  ur_event_handle_t *Event = phEvent ? phEvent : &InternalEvent;
  ur_command_t CmdType = (imageCopyFlags == UR_EXP_IMAGE_COPY_FLAG_HOST_TO_DEVICE) ? UR_COMMAND_BINDLESS_IMAGE_MEM_COPY_FROM_HOST : UR_COMMAND_BINDLESS_IMAGE_MEM_COPY_TO_HOST;
  UR_CALL(createEventAndAssociateQueue(hQueue, Event, CmdType, CommandList,
                                       IsInternal, /*IsMultiDevice*/ false));
  ZeEvent = (*Event)->ZeEvent;
  (*Event)->WaitList = TmpWaitList;

  const auto &ZeCommandList = CommandList->first;
  const auto &WaitList = (*Event)->WaitList;

  if (imageCopyFlags == UR_EXP_IMAGE_COPY_FLAG_HOST_TO_DEVICE) {
    ze_image_region_t DstRegion;
    UR_CALL(getImageRegionHelper(ZeImageDesc, &dstOffset, &copyExtent, DstRegion));
    uint32_t srcRowPitch = 0;
    uint32_t srcSlicePitch = 0;

    auto *ZeImageMem = reinterpret_cast<ze_image_mem_handle_exp_t>(pDst);
    ZE2UR_CALL(zeCommandListAppendImageMemoryCopyFromHostExp,
              (ZeCommandList, ZeImageMem, pSrc, &DstRegion, srcRowPitch, srcSlicePitch,
                ZeEvent, WaitList.Length, WaitList.ZeEventList));
  } else {
    ze_image_region_t SrcRegion;
    UR_CALL(getImageRegionHelper(ZeImageDesc, &srcOffset, &copyExtent, SrcRegion));
    auto *ZeImageMem = reinterpret_cast<ze_image_mem_handle_exp_t>(pSrc);
    uint32_t dstRowPitch = 0;
    uint32_t dstSlicePitch = 0;
    ZE2UR_CALL(zeCommandListAppendImageMemoryCopyToHostExp,
              (ZeCommandList, pDst, ZeImageMem, &SrcRegion, dstRowPitch, dstSlicePitch,
                ZeEvent, WaitList.Length, WaitList.ZeEventList));
  }
  printf("[urBindlessImagesImageCopyExp] END, %s, Dst: %p, Src: %p, copyExtent: {%ld, %ld, %ld}, hostExtent: {%ld, %ld, %ld}\n\n",
      (imageCopyFlags == UR_EXP_IMAGE_COPY_FLAG_HOST_TO_DEVICE) ? "CopyFromHost" : "CopyToHost", pDst, pSrc,
      copyExtent.width, copyExtent.height, copyExtent.depth, hostExtent.width, hostExtent.height, hostExtent.depth);

  UR_CALL(hQueue->executeCommandList(CommandList, /*Blocking*/ true, OkToBatch));

  return UR_RESULT_SUCCESS;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesImageGetInfoExp(
    ur_exp_image_mem_handle_t hImageMem, ur_image_info_t propName,
    void *pPropValue, size_t *pPropSizeRet) {
  std::ignore = hImageMem;
  std::ignore = propName;
  std::ignore = pPropValue;
  std::ignore = pPropSizeRet;
  urPrint("[UR][L0] %s function not implemented!\n", __FUNCTION__);
  return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesMipmapGetLevelExp(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    ur_exp_image_mem_handle_t hImageMem, uint32_t mipmapLevel,
    ur_exp_image_mem_handle_t *phImageMem) {
  std::ignore = hContext;
  std::ignore = hDevice;
  std::ignore = hImageMem;
  std::ignore = mipmapLevel;
  std::ignore = phImageMem;
  urPrint("[UR][L0] %s function not implemented!\n", __FUNCTION__);
  return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesMipmapFreeExp(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    ur_exp_image_mem_handle_t hMem) {
  std::ignore = hContext;
  std::ignore = hDevice;
  std::ignore = hMem;
  urPrint("[UR][L0] %s function not implemented!\n", __FUNCTION__);
  return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesImportOpaqueFDExp(
    ur_context_handle_t hContext, ur_device_handle_t hDevice, size_t size,
    ur_exp_interop_mem_desc_t *pInteropMemDesc,
    ur_exp_interop_mem_handle_t *phInteropMem) {
  std::ignore = hContext;
  std::ignore = hDevice;
  std::ignore = size;
  std::ignore = pInteropMemDesc;
  std::ignore = phInteropMem;
  urPrint("[UR][L0] %s function not implemented!\n", __FUNCTION__);
  return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesMapExternalArrayExp(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    const ur_image_format_t *pImageFormat, const ur_image_desc_t *pImageDesc,
    ur_exp_interop_mem_handle_t hInteropMem,
    ur_exp_image_mem_handle_t *phImageMem) {
  std::ignore = hContext;
  std::ignore = hDevice;
  std::ignore = pImageFormat;
  std::ignore = pImageDesc;
  std::ignore = hInteropMem;
  std::ignore = phImageMem;
  urPrint("[UR][L0] %s function not implemented!\n", __FUNCTION__);
  return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesReleaseInteropExp(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    ur_exp_interop_mem_handle_t hInteropMem) {
  std::ignore = hContext;
  std::ignore = hDevice;
  std::ignore = hInteropMem;
  urPrint("[UR][L0] %s function not implemented!\n", __FUNCTION__);
  return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

UR_APIEXPORT ur_result_t UR_APICALL
urBindlessImagesImportExternalSemaphoreOpaqueFDExp(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    ur_exp_interop_semaphore_desc_t *pInteropSemaphoreDesc,
    ur_exp_interop_semaphore_handle_t *phInteropSemaphoreHandle) {
  std::ignore = hContext;
  std::ignore = hDevice;
  std::ignore = pInteropSemaphoreDesc;
  std::ignore = phInteropSemaphoreHandle;
  urPrint("[UR][L0] %s function not implemented!\n", __FUNCTION__);
  return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesDestroyExternalSemaphoreExp(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    ur_exp_interop_semaphore_handle_t hInteropSemaphore) {
  std::ignore = hContext;
  std::ignore = hDevice;
  std::ignore = hInteropSemaphore;
  urPrint("[UR][L0] %s function not implemented!\n", __FUNCTION__);
  return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesWaitExternalSemaphoreExp(
    ur_queue_handle_t hQueue, ur_exp_interop_semaphore_handle_t hSemaphore,
    uint32_t numEventsInWaitList, const ur_event_handle_t *phEventWaitList,
    ur_event_handle_t *phEvent) {
  std::ignore = hQueue;
  std::ignore = hSemaphore;
  std::ignore = numEventsInWaitList;
  std::ignore = phEventWaitList;
  std::ignore = phEvent;
  urPrint("[UR][L0] %s function not implemented!\n", __FUNCTION__);
  return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
}

UR_APIEXPORT ur_result_t UR_APICALL urBindlessImagesSignalExternalSemaphoreExp(
    ur_queue_handle_t hQueue, ur_exp_interop_semaphore_handle_t hSemaphore,
    uint32_t numEventsInWaitList, const ur_event_handle_t *phEventWaitList,
    ur_event_handle_t *phEvent) {
  std::ignore = hQueue;
  std::ignore = hSemaphore;
  std::ignore = numEventsInWaitList;
  std::ignore = phEventWaitList;
  std::ignore = phEvent;
  urPrint("[UR][L0] %s function not implemented!\n", __FUNCTION__);
  return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
}
