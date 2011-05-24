#pragma once

#include "renderer.hpp"

#include "../geometry/point2d.hpp"

#include "../std/vector.hpp"

namespace yg
{
  namespace gl
  {
    class BaseTexture;

    /// all the layers are combined by a simple blitting
    /// at the endFrame/updateActualTarget.
    class LayerManager : public Renderer
    {
    public:

      struct Layer
      {
        shared_ptr<BaseTexture> m_surface;
        m2::PointU m_org;
      };

    private:

      typedef vector<Layer> TLayers;
      TLayers m_layers;

    public:

      typedef Renderer base_t;

      LayerManager(base_t::Params const & params);
/*      /// adding composition layer. it's up to the higher levels
      /// of the hierarchy to use this layers for composition.
      int addLayer(shared_ptr<BaseTexture> const & layer, m2::PointU const & org = m2::PointU(0, 0));
      /// remove layer from composition stack
      void removeLayer(int idx);
      /// get specific layer
      Layer const & layer(int idx) const;
      /// get layers count
      int  layersCount() const;*/
    };
  }
}
