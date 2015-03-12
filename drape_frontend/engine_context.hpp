#pragma once

#include "drape_frontend/threads_commutator.hpp"

#include "drape/pointers.hpp"

namespace df
{

class Message;
enum class MessagePriority;
class MapShape;
struct TileKey;

class EngineContext
{
public:
  EngineContext(dp::RefPointer<ThreadsCommutator> commutator);

  void BeginReadTile(TileKey const & key);
  /// If you call this method, you may forget about shape.
  /// It will be proccessed and delete later
  void InsertShape(TileKey const & key, dp::TransferPointer<MapShape> shape);
  void EndReadTile(TileKey const & key);

private:
  void PostMessage(Message * message, MessagePriority priority);

private:
  dp::RefPointer<ThreadsCommutator> m_commutator;
};

} // namespace df
