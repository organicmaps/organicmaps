#include "clustering.hpp"

Cluster::Cluster(POI poi)
{
  pois.push_back(poi);
  center = poi.coords;
}

void Cluster::mergeCluster(Cluster& other)
{
  pois.insert(pois.end(), other.pois.begin(), other.pois.end());
  updateCenter();
}

void Cluster::updateCenter()
{
  m2::PointD sum(0, 0);
  for (const POI& poi : pois)
    sum += poi.coords;

  center = sum / pois.size();
}

double Cluster::distanceTo(const Cluster& other) const
{
  return center.Length(other.center);
}

void hierarchicalGreedyClustering(double threshold, std::vector<Cluster>& clusters)
{
  bool reloop = false;

  int i = 0;
  int j;
  while (true)
  {
    if (i == clusters.size() - 1)
      break;
    j = i + 1;

    while (true)
    {
      if (j == clusters.size() - 1)
        break;

      double distance = clusters[i].distanceTo(clusters[j]);

      if (distance < threshold)
      {
        clusters[i].mergeCluster(clusters[j]);
        clusters.erase(clusters.begin() + j);
        reloop = true;
      }
      else
        ++j;
    }
    ++i;
  }
  if (reloop)
    hierarchicalGreedyClustering(threshold, clusters);

  return;
}
