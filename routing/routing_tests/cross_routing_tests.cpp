#include "../../testing/testing.hpp"

#include "../../routing/cross_routing_context.hpp"
#include "../../coding/reader.hpp"
#include "../../coding/writer.hpp"


namespace
{
UNIT_TEST(TestContextSerialization)
{
  routing::CrossRoutingContext context, newContext;

  context.addIngoingNode(1);
  context.addIngoingNode(2);
  context.addOutgoingNode(3, "foo");
  context.addOutgoingNode(4, "bar");
  context.reserveAdjacencyMatrix();

  vector<char> buffer;
  MemWriter<vector<char> > writer(buffer);
  context.Save(writer);
  TEST_GREATER(buffer.size(), 5, ("Context serializer make some data"));

  MemReader reader(buffer.data(), buffer.size());
  newContext.Load(reader);
  auto ins = newContext.GetIngoingIterators();
  TEST_EQUAL(distance(ins.first,ins.second), 2, ());
  TEST_EQUAL(*ins.first, 1, ());
  TEST_EQUAL(*(++ins.first), 2, ());

  auto outs = newContext.GetOutgoingIterators();
  TEST_EQUAL(distance(outs.first,outs.second), 2, ());
  TEST_EQUAL(outs.first->first, 3, ());
  TEST_EQUAL(newContext.getOutgoingMwmName(outs.first->second), string("foo"), ());
  ++outs.first;
  TEST_EQUAL(outs.first->first, 4, ());
  TEST_EQUAL(newContext.getOutgoingMwmName(outs.first->second), string("bar"), ());
}
}
