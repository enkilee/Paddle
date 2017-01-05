#include "PaddleCAPI.h"
#include "PaddleCAPIPrivate.h"
#include "hl_cuda.h"

#define cast(v) paddle::capi::cast<paddle::capi::CMatrix>(v)
extern "C" {
int PDMatCreate(PD_Matrix* mat, uint64_t height, uint64_t width, bool useGpu) {
  auto ptr = new paddle::capi::CMatrix();
  ptr->mat = paddle::Matrix::create(height, width, false, useGpu);
  *mat = ptr;
  return kPD_NO_ERROR;
}

int PDMatCreateNone(PD_Matrix* mat) {
  auto ptr = new paddle::capi::CMatrix();
  *mat = ptr;
  return kPD_NO_ERROR;
}

int PDMatDestroy(PD_Matrix mat) {
  if (mat == nullptr) return kPD_NULLPTR;
  auto ptr = cast(mat);
  delete ptr;
  return kPD_NO_ERROR;
}

int PDMatCopyToRow(PD_Matrix mat, uint64_t rowID, pd_real* rowArray) {
  if (mat == nullptr) return kPD_NULLPTR;
  auto ptr = cast(mat);
  if (ptr->mat == nullptr) return kPD_NULLPTR;
  if (rowID >= ptr->mat->getHeight()) return kPD_OUT_OF_RANGE;
  paddle::real* buf = ptr->mat->getRowBuf(rowID);
  size_t width = ptr->mat->getWidth();
#ifndef PADDLE_ONLY_CPU
  hl_memcpy(buf, rowArray, sizeof(paddle::real) * width);
#else
  std::copy(rowArray, rowArray + width, buf);
#endif
  return kPD_NO_ERROR;
}

int PDMatGetRow(PD_Matrix mat, uint64_t rowID, pd_real** rawRowBuffer) {
  if (mat == nullptr) return kPD_NULLPTR;
  auto ptr = cast(mat);
  if (ptr->mat == nullptr) return kPD_NULLPTR;
  if (rowID >= ptr->mat->getHeight()) return kPD_OUT_OF_RANGE;
  *rawRowBuffer = ptr->mat->getRowBuf(rowID);
  return kPD_NO_ERROR;
}

int PDMatGetShape(PD_Matrix mat, uint64_t* height, uint64_t* width) {
  if (mat == nullptr) return kPD_NULLPTR;
  if (height != nullptr) {
    *height = cast(mat)->mat->getHeight();
  }
  if (width != nullptr) {
    *width = cast(mat)->mat->getWidth();
  }
  return kPD_NO_ERROR;
}
}
