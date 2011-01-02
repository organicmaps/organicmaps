#include "../base/SRC_FIRST.hpp"

#include "layer_manager.hpp"

namespace yg
{
  namespace gl
  {
    LayerManager::LayerManager(base_t::Params const & params) : base_t(params)
    {}

/*    int LayerManager::addLayer(shared_ptr<BaseTexture> const & layer, m2::PointU const & org)
    {
      Layer l = {layer, org};
      m_layers.push_back(l);
      return m_layers.size() - 1;
    }

    void LayerManager::removeLayer(int idx)
    {
      TLayers::iterator it = m_layers.begin();
      advance(it, idx);
      m_layers.erase(it);
    }

    LayerManager::Layer const & LayerManager::layer(int idx) const
    {
      return m_layers[idx];
    }

    int LayerManager::layersCount() const
    {
      return m_layers.size();
    }*/
  }
}
