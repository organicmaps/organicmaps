#include "network/socket.hpp"

namespace om::network
{
class DummySocket final : public Socket
{
public:
  bool Open(std::string const &, uint16_t) override { return false; }
  void Close() override {}
  bool Read(uint8_t *, uint32_t) override { return false; }
  bool Write(uint8_t const *, uint32_t) override { return false; }
  void SetTimeout(uint32_t) override {}
};

std::unique_ptr<Socket> CreateSocket() { return std::make_unique<DummySocket>(); }
}  // namespace om::network
