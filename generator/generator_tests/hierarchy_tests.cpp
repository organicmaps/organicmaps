#include "testing/testing.hpp"

#include "generator/feature_builder.hpp"
#include "generator/filter_complex.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"
#include "generator/hierarchy.hpp"

#include "indexer/classificator.hpp"

#include "base/stl_helpers.hpp"

#include <iterator>
#include <vector>

namespace
{
using namespace generator::tests_support;

std::vector<feature::FeatureBuilder> MakeTestSet1()
{
  std::vector<feature::FeatureBuilder> fbs;
  {
    feature::FeatureBuilder outline;
    outline.AddPolygon({{0.0, 0.0}, {0.0, 10.0}, {10.0, 10.0}, {10.0, 0.0}});
    outline.AddOsmId(base::MakeOsmWay(1));
    outline.AddType(classif().GetTypeByPath({"building"}));
    outline.AddType(classif().GetTypeByPath({"tourism", "attraction"}));
    outline.SetArea();
    fbs.emplace_back(outline);
  }
  {
    feature::FeatureBuilder buildingPart;
    buildingPart.AddPolygon({{0.0, 0.0}, {0.0, 8.0}, {8.0, 8.0}, {8.0, 0.0}});
    buildingPart.AddOsmId(base::MakeOsmWay(2));
    buildingPart.AddType(classif().GetTypeByPath({"building:part"}));
    buildingPart.SetArea();
    fbs.emplace_back(buildingPart);
  }
  {
    feature::FeatureBuilder buildingPart;
    buildingPart.AddPolygon({{0.0, 0.0}, {0.0, 7.0}, {7.0, 7.0}, {7.0, 0.0}});
    buildingPart.AddOsmId(base::MakeOsmWay(3));
    buildingPart.AddType(classif().GetTypeByPath({"building:part"}));
    buildingPart.SetArea();
    fbs.emplace_back(buildingPart);
  }
  {
    feature::FeatureBuilder buildingPart;
    buildingPart.AddPolygon({{0.0, 0.0}, {0.0, 6.0}, {6.0, 6.0}, {6.0, 0.0}});
    buildingPart.AddOsmId(base::MakeOsmWay(4));
    buildingPart.AddType(classif().GetTypeByPath({"building:part"}));
    buildingPart.SetArea();
    fbs.emplace_back(buildingPart);
  }

  return fbs;
}

void TestDepthNodeById(generator::hierarchy::HierarchyLinker::Node::Ptr const & tree, uint64_t id, size_t depth)
{
  auto osmId = base::MakeOsmWay(id);
  TEST_EQUAL(tree_node::GetDepth(
                 tree_node::FindIf(tree, [&](auto const & data) { return data.GetCompositeId().m_mainId == osmId; })),
             depth, ());
}

UNIT_CLASS_TEST(TestWithClassificator, ComplexHierarchy_FlattenBuildingParts)
{
  auto trees = generator::hierarchy::BuildHierarchy(MakeTestSet1(), generator::hierarchy::GetMainType,
                                                    std::make_shared<generator::FilterComplex>());
  TEST_EQUAL(trees.size(), 1, ());
  TEST_EQUAL(tree_node::Size(trees[0]), 4, ());

  TestDepthNodeById(trees[0], 2 /* id */, 2 /* depth */);
  TestDepthNodeById(trees[0], 3 /* id */, 3 /* depth */);
  TestDepthNodeById(trees[0], 4 /* id */, 4 /* depth */);

  generator::hierarchy::FlattenBuildingParts(trees);

  TEST_EQUAL(trees.size(), 1, ());
  TEST_EQUAL(tree_node::Size(trees[0]), 4, ());

  TestDepthNodeById(trees[0], 2 /* id */, 2 /* depth */);
  TestDepthNodeById(trees[0], 3 /* id */, 2 /* depth */);
  TestDepthNodeById(trees[0], 4 /* id */, 2 /* depth */);
}

UNIT_CLASS_TEST(TestWithClassificator, ComplexHierarchy_AddChildrenTo)
{
  auto trees = generator::hierarchy::BuildHierarchy(MakeTestSet1(), generator::hierarchy::GetMainType,
                                                    std::make_shared<generator::FilterComplex>());
  TEST_EQUAL(trees.size(), 1, ());
  TEST_EQUAL(tree_node::Size(trees[0]), 4, ());

  auto osmId = base::MakeOsmWay(3);
  generator::hierarchy::AddChildrenTo(trees, [&](auto const & compositeId)
  {
    std::vector<generator::hierarchy::HierarchyPlace> fbs;
    if (compositeId.m_mainId == osmId)
    {
      feature::FeatureBuilder buildingPart;
      buildingPart.AddPolygon({{6.0, 6.0}, {6.0, 7.0}, {7.0, 7.0}, {7.0, 6.0}});
      buildingPart.AddOsmId(base::MakeOsmWay(5));
      buildingPart.AddType(classif().GetTypeByPath({"building:part"}));
      buildingPart.SetArea();
      fbs.emplace_back(generator::hierarchy::HierarchyPlace(buildingPart));
    }
    return fbs;
  });

  auto node = tree_node::FindIf(trees[0], [&](auto const & data) { return data.GetCompositeId().m_mainId == osmId; });

  TEST(node, ());

  auto const children = node->GetChildren();
  TEST_EQUAL(children.size(), 2, ());

  auto it = base::FindIf(
      children, [](auto const & node) { return node->GetData().GetCompositeId().m_mainId == base::MakeOsmWay(4); });
  CHECK(it != std::end(children), ());
  CHECK((*it)->HasParent() && (*it)->GetParent()->GetData().GetCompositeId().m_mainId == osmId, ());

  it = base::FindIf(children,
                    [](auto const & node) { return node->GetData().GetCompositeId().m_mainId == base::MakeOsmWay(5); });
  CHECK(it != std::end(children), ());
  CHECK((*it)->HasParent() && (*it)->GetParent()->GetData().GetCompositeId().m_mainId == osmId, ());
}
}  // namespace
