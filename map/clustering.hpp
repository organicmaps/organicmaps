#pragma once

#include "geometry/point2d.hpp"
#include <vector>

// Define POI structure
struct POI
{
  int id;
  m2::PointD coords; // screen coordinates
};

// Define Cluster structure
struct Cluster
{
  std::vector<POI> pois;
  m2::PointD center;

  Cluster(POI poi);

  void mergeCluster(Cluster& other);

  void updateCenter();

  double distanceTo(const Cluster& other) const;
};

// Hierarchical Greedy Clustering
void hierarchicalGreedyClustering(double threshold, std::vector<Cluster>& clusters);
