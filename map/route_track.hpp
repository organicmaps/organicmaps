#pragma once

#include "map/track.hpp"

#include "routing/turns.hpp"
#include "drape/drape_global.hpp"

#include "platform/location.hpp"

class RouteTrack : public Track
{
  RouteTrack & operator=(RouteTrack const &) = delete;
  RouteTrack(RouteTrack const &) = delete;
public:
  RouteTrack() {}
  explicit RouteTrack(PolylineD const & polyline) : Track(polyline) {}
  virtual ~RouteTrack();
//  virtual void CreateDisplayList(graphics::Screen * dlScreen, MatrixT const & matrix, bool isScaleChanged,
//                                int drawScale, double visualScale,
//                                location::RouteMatchingInfo const & matchingInfo) const;
//  virtual void Draw(graphics::Screen * pScreen, MatrixT const & matrix) const;
  virtual RouteTrack * CreatePersistent();
//  virtual void CleanUp() const;
//  virtual bool HasDisplayLists() const;

  void AddClosingSymbol(bool isBeginSymbol, string const & symbolName,
                        dp::Anchor pos, double depth);

  void SetArrowColor(graphics::Color color) { m_arrowColor = color; }

private:
  //void CreateDisplayListSymbols(graphics::Screen * dlScreen, PointContainerT const & pts) const;

  //void CreateDisplayListArrows(graphics::Screen * dlScreen, MatrixT const & matrix, double visualScale) const;
  void DeleteClosestSegmentDisplayList() const;
  //bool HasClosestSegmentDisplayList() const { return m_closestSegmentDL != nullptr; }
  //void SetClosestSegmentDisplayList(graphics::DisplayList * dl) const { m_closestSegmentDL = dl; }
  void Swap(RouteTrack & rhs);

  struct ClosingSymbol
  {
    ClosingSymbol(string const & iconName, dp::Anchor pos, double depth)
      : m_iconName(iconName), m_position(pos), m_depth(depth) {}
    string m_iconName;
    dp::Anchor m_position;
    double m_depth;
  };

  vector<ClosingSymbol> m_beginSymbols;
  vector<ClosingSymbol> m_endSymbols;

  mutable location::RouteMatchingInfo m_relevantMatchedInfo;
  /// @TODO UVR
  //mutable graphics::DisplayList * m_closestSegmentDL = nullptr;
};

bool ClipArrowBodyAndGetArrowDirection(vector<m2::PointD> & ptsTurn, pair<m2::PointD, m2::PointD> & arrowDirection,
                                       size_t turnIndex, double beforeTurn, double afterTurn, double arrowLength);
bool MergeArrows(vector<m2::PointD> & ptsCurrentTurn, vector<m2::PointD> const & ptsNextTurn, double bodyLen, double arrowLen);
