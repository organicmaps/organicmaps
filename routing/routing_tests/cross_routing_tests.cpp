#include "testing/testing.hpp"

#include "routing/cross_routing_context.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"


namespace
{
UNIT_TEST(TestContextSerialization)
{
  routing::CrossRoutingContextWriter context;
  routing::CrossRoutingContextReader newContext;

  context.AddIngoingNode(1, m2::PointD::Zero());
  context.AddIngoingNode(2, m2::PointD::Zero());
  context.AddOutgoingNode(3, "foo", m2::PointD::Zero());
  context.AddOutgoingNode(4, "bar", m2::PointD::Zero());
  context.ReserveAdjacencyMatrix();

  vector<char> buffer;
  MemWriter<vector<char> > writer(buffer);
  context.Save(writer);
  TEST_GREATER(buffer.size(), 5, ("Context serializer make some data"));

  MemReader reader(buffer.data(), buffer.size());
  newContext.Load(reader);
  auto ins = newContext.GetIngoingIterators();
  TEST_EQUAL(distance(ins.first,ins.second), 2, ());
  TEST_EQUAL(ins.first->m_nodeId, 1, ());
  TEST_EQUAL((++ins.first)->m_nodeId, 2, ());

  auto outs = newContext.GetOutgoingIterators();
  TEST_EQUAL(distance(outs.first,outs.second), 2, ());
  TEST_EQUAL(outs.first->m_nodeId, 3, ());
  TEST_EQUAL(newContext.GetOutgoingMwmName(*outs.first), string("foo"), ());
  ++outs.first;
  TEST_EQUAL(outs.first->m_nodeId, 4, ());
  TEST_EQUAL(newContext.GetOutgoingMwmName(*outs.first), string("bar"), ());
}

UNIT_TEST(TestAdjacencyMatrix)
{
  routing::CrossRoutingContextWriter context;
  routing::CrossRoutingContextReader newContext;

  context.AddIngoingNode(1, m2::PointD::Zero());
  context.AddIngoingNode(2, m2::PointD::Zero());
  context.AddIngoingNode(3, m2::PointD::Zero());
  context.AddOutgoingNode(4, "foo", m2::PointD::Zero());
  context.ReserveAdjacencyMatrix();
  {
    auto ins = context.GetIngoingIterators();
    auto outs = context.GetOutgoingIterators();
    context.SetAdjacencyCost(ins.first, outs.first, 5);
    context.SetAdjacencyCost(ins.first + 1, outs.first, 9);
  }

  vector<char> buffer;
  MemWriter<vector<char> > writer(buffer);
  context.Save(writer);
  TEST_GREATER(buffer.size(), 5, ("Context serializer make some data"));

  MemReader reader(buffer.data(), buffer.size());
  newContext.Load(reader);
  auto ins = newContext.GetIngoingIterators();
  auto outs = newContext.GetOutgoingIterators();
  TEST_EQUAL(newContext.GetAdjacencyCost(ins.first, outs.first), 5, ());
  TEST_EQUAL(newContext.GetAdjacencyCost(ins.first + 1, outs.first), 9, ());
  TEST_EQUAL(newContext.GetAdjacencyCost(ins.first + 2, outs.first),
             routing::INVALID_CONTEXT_EDGE_WEIGHT, ("Default cost"));
}

}
