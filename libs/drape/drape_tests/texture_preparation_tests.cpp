#include "testing/testing.hpp"

#include "drape/texture.hpp"

namespace
{
class TestHardwareTexture final : public dp::HWTexture
{
public:
  TestHardwareTexture() { m_target = 0x0DE1; }  // GL_TEXTURE_2D, without a GPU context.
  void Create(ref_ptr<dp::GraphicsContext>, Params const &, ref_ptr<void>) override {}
  void UploadData(ref_ptr<dp::GraphicsContext>, uint32_t, uint32_t, uint32_t, uint32_t, ref_ptr<void>) override {}
  void UploadData(ref_ptr<dp::GraphicsContext>, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                  ref_ptr<void>) override
  {}
  void Bind(ref_ptr<dp::GraphicsContext>) const override {}
  void SetFilter(dp::TextureFilter) override {}
  bool Validate() const override { return true; }
};

class LazyTexture final : public dp::Texture
{
public:
  int m_updates = 0;
  ref_ptr<ResourceInfo> FindResource(Key const &, bool &) override { return nullptr; }
  void UpdateState(ref_ptr<dp::GraphicsContext>) override
  {
    ++m_updates;
    m_hwTexture = make_unique_dp<TestHardwareTexture>();
  }
  void Release() { Destroy(); }
};
}  // namespace

UNIT_TEST(Texture_PreparesLateDynamicTextureBeforeAccessingGpuTarget)
{
  LazyTexture texture;
  TEST(texture.GetHardwareTexture() == nullptr, ());
  auto hardware = texture.PrepareForRendering(nullptr);
  TEST(hardware != nullptr, ());
  TEST_EQUAL(hardware->GetTarget(), 0x0DE1, ());
  TEST_EQUAL(texture.m_updates, 1, ());
  TEST(texture.PrepareForRendering(nullptr) == hardware, ());
  TEST_EQUAL(texture.m_updates, 1, ("An existing GPU texture must not be uploaded again"));
  texture.Release();
  TEST(texture.GetHardwareTexture() == nullptr, ());
  TEST(texture.PrepareForRendering(nullptr) != nullptr, ());
  TEST_EQUAL(texture.m_updates, 2, ());
}
