#ifndef SPECK1D_FLT_H
#define SPECK1D_FLT_H

#include "SPECK_FLT.h"

namespace sperr {

class SPECK1D_FLT : public SPECK_FLT {
 protected:
  void m_instantiate_encoder() override;
  void m_instantiate_decoder() override;

  void m_wavelet_xform() override;
  void m_inverse_wavelet_xform(bool) override;
};

}  // namespace sperr

#endif
