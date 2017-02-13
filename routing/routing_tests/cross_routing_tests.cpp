#include "testing/testing.hpp"

#include "routing/cross_mwm_road_graph.hpp"
#include "routing/cross_mwm_router.hpp"
#include "routing/cross_routing_context.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

using namespace routing;

namespace
{
// Graph to convertions.
UNIT_TEST(TestCrossRouteConverter)
{
  Index::MwmId aMap, bMap;
  vector<BorderCross> graphCrosses;
  CrossNode const a(1, aMap, {0, 0}), b(2, aMap, {2, 2});
  CrossNode const c(3, bMap, {3, 3}), d(3, bMap, {4, 4});
  graphCrosses.emplace_back(a, b);
  graphCrosses.emplace_back(b, c);
  graphCrosses.emplace_back(c, d);
  FeatureGraphNode startGraphNode;
  startGraphNode.node.forward_node_id = 5;
  startGraphNode.mwmId = aMap;
  FeatureGraphNode finalGraphNode;
  finalGraphNode.node.reverse_node_id = 6;
  finalGraphNode.mwmId = bMap;
  TCheckedPath route;
  ConvertToSingleRouterTasks(graphCrosses, startGraphNode, finalGraphNode, route);
  TEST_EQUAL(route.size(), 2, ("We have 2 maps aMap and bMap."));
  TEST_EQUAL(route.front().startNode.node, startGraphNode.node,
             ("Start node must be replaced by origin."));
  TEST_EQUAL(route.back().finalNode.node, finalGraphNode.node,
             ("End node must be replaced by origin."));
}

// Cross routing context tests.
UNIT_TEST(TestContextSerialization)
{
  routing::CrossRoutingContextWriter context;
  routing::CrossRoutingContextReader newContext;

  context.AddIngoingNode(1, ms::LatLon::Zero());
  context.AddIngoingNode(2, ms::LatLon::Zero());
  context.AddOutgoingNode(3, "foo", ms::LatLon::Zero());
  context.AddOutgoingNode(4, "bar", ms::LatLon::Zero());
  context.ReserveAdjacencyMatrix();

  vector<char> buffer;
  MemWriter<vector<char> > writer(buffer);
  context.Save(writer);
  TEST_GREATER(buffer.size(), 5, ("Context serializer make some data"));

  MemReader reader(buffer.data(), buffer.size());
  newContext.Load(reader);
  vector<IngoingCrossNode> ingoingNodes;
  newContext.ForEachIngoingNode([&ingoingNodes](IngoingCrossNode const & node)
                                {
                                  ingoingNodes.push_back(node);
                                });
  TEST_EQUAL(ingoingNodes.size(), 2, ());
  TEST_EQUAL(ingoingNodes[0].m_nodeId, 1, ());
  TEST_EQUAL(ingoingNodes[1].m_nodeId, 2, ());

  vector<OutgoingCrossNode> outgoingNodes;
  newContext.ForEachOutgoingNode([&outgoingNodes](OutgoingCrossNode const & node)
                                 {
                                   outgoingNodes.push_back(node);
                                 });
  TEST_EQUAL(outgoingNodes.size(), 2, ());
  TEST_EQUAL(outgoingNodes[0].m_nodeId, 3, ());
  TEST_EQUAL(newContext.GetOutgoingMwmName(outgoingNodes[0]), string("foo"), ());
  TEST_EQUAL(outgoingNodes[1].m_nodeId, 4, ());
  TEST_EQUAL(newContext.GetOutgoingMwmName(outgoingNodes[1]), string("bar"), ());
}

UNIT_TEST(TestAdjacencyMatrix)
{
  routing::CrossRoutingContextWriter context;
  routing::CrossRoutingContextReader newContext;

  context.AddIngoingNode(1, ms::LatLon::Zero());
  context.AddIngoingNode(2, ms::LatLon::Zero());
  context.AddIngoingNode(3, ms::LatLon::Zero());
  context.AddOutgoingNode(4, "foo", ms::LatLon::Zero());
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
  vector<IngoingCrossNode> ingoingNodes;
  newContext.ForEachIngoingNode([&ingoingNodes](IngoingCrossNode const & node)
                                {
                                  ingoingNodes.push_back(node);
                                });
  vector<OutgoingCrossNode> outgoingNodes;
  newContext.ForEachOutgoingNode([&outgoingNodes](OutgoingCrossNode const & node)
                                 {
                                   outgoingNodes.push_back(node);
                                 });
  TEST_EQUAL(newContext.GetAdjacencyCost(ingoingNodes[0], outgoingNodes[0]), 5, ());
  TEST_EQUAL(newContext.GetAdjacencyCost(ingoingNodes[1], outgoingNodes[0]), 9, ());
  TEST_EQUAL(newContext.GetAdjacencyCost(ingoingNodes[2], outgoingNodes[0]),
             routing::kInvalidContextEdgeWeight, ("Default cost"));
}

UNIT_TEST(TestFindingByPoint)
{
  routing::CrossRoutingContextWriter context;
  routing::CrossRoutingContextReader newContext;

  ms::LatLon p1(1., 1.), p2(5., 5.), p3(10., 1.), p4(20., 1.);

  context.AddIngoingNode(1, ms::LatLon::Zero());
  context.AddIngoingNode(2, p1);
  context.AddIngoingNode(3, p2);
  context.AddOutgoingNode(4, "foo", ms::LatLon::Zero());
  context.AddIngoingNode(5, p3);
  context.AddIngoingNode(6, p3);
  context.ReserveAdjacencyMatrix();

  vector<char> buffer;
  MemWriter<vector<char> > writer(buffer);
  context.Save(writer);
  TEST_GREATER(buffer.size(), 5, ("Context serializer make some data"));

  MemReader reader(buffer.data(), buffer.size());
  newContext.Load(reader);
  vector<IngoingCrossNode> node;
  auto fn = [&node](IngoingCrossNode const & nd) {node.push_back(nd);};
  TEST(newContext.ForEachIngoingNodeNearPoint(p1, fn), ());
  TEST_EQUAL(node.size(), 1, ());
  TEST_EQUAL(node[0].m_nodeId, 2, ());
  node.clear();
  TEST(newContext.ForEachIngoingNodeNearPoint(p2, fn), ());
  TEST_EQUAL(node.size(), 1, ());
  TEST_EQUAL(node[0].m_nodeId, 3, ());
  node.clear();
  TEST(!newContext.ForEachIngoingNodeNearPoint(p4, fn), ());
  node.clear();
  TEST(newContext.ForEachIngoingNodeNearPoint(p3, fn), ());
  TEST_EQUAL(node.size(), 2, ());
  TEST_EQUAL(node[0].m_nodeId, 5, ());
  TEST_EQUAL(node[1].m_nodeId, 6, ());

  vector<OutgoingCrossNode> outgoingNode;
  auto fnOutgoing = [&outgoingNode](OutgoingCrossNode const & nd) {outgoingNode.push_back(nd);};
  TEST(newContext.ForEachOutgoingNodeNearPoint(ms::LatLon::Zero(), fnOutgoing), ());
  TEST_EQUAL(outgoingNode.size(), 1, ());
  TEST_EQUAL(outgoingNode[0].m_nodeId, 4, ());

  outgoingNode.clear();
  TEST(!newContext.ForEachOutgoingNodeNearPoint(p3, fnOutgoing), ());
  TEST(outgoingNode.empty(), ());
}
}  // namespace
