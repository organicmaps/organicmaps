#include <utility>

#include "routing/routing_quality/routing_quality_tool/utils.hpp"

#include "routing/vehicle_mask.hpp"

#include "kml/serdes.hpp"
#include "kml/types.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <iomanip>

using namespace routing::routes_builder;

namespace
{
using namespace routing_quality;

void PrintWithSpaces(std::string const & str, size_t maxN)
{
  std::cout << str;
  if (maxN <= str.size())
    return;

  maxN -= str.size();
  for (size_t i = 0; i < maxN; ++i)
    std::cout << " ";
}

std::vector<m2::PointD> ConvertToPointDVector(std::vector<ms::LatLon> const & latlons)
{
  std::vector<m2::PointD> result(latlons.size());
  for (size_t i = 0; i < latlons.size(); ++i)
    result[i] = mercator::FromLatLon(latlons[i]);

  return result;
}

std::string MakeStringFromPercent(double percent)
{
  std::stringstream ss;
  ss << std::setprecision(2);
  ss << percent;
  std::string percentStr;
  ss >> percentStr;
  percentStr += "%";

  return percentStr;
}

kml::BookmarkData CreateBookmark(m2::PointD const & point, bool isStart)
{
  kml::BookmarkData bookmarkData;
  bookmarkData.m_color = {isStart ? kml::PredefinedColor::Red : kml::PredefinedColor::Blue, 0};
  bookmarkData.m_point = point;

  return bookmarkData;
}

template <typename AnotherResult>
void SaveKmlFileDataTo(RoutesBuilder::Result const & mapsmeResult,
                       AnotherResult const & apiResult,
                       std::string const & kmlFile)
{
  static std::vector<uint32_t> const kColors = {
      0xff0000ff,  // Red
      0x0000ffff,  // Blue
      0x00ff00ff,  // Green
      0xffa500ff,  // Orange
      0xa52a2aff   // Brown
  };

  kml::FileData kml;

  size_t colorNumber = 0;
  auto const addTrack = [&](kml::TrackData && track) {
    CHECK_LESS(colorNumber, kColors.size(), ());
    track.m_layers = {{5.0 /* lineWidth */, {kml::PredefinedColor::None, kColors[colorNumber++]}}};
    kml.m_tracksData.emplace_back(std::move(track));
  };

  auto const & start = apiResult.GetStartPoint();
  auto const & finish = apiResult.GetFinishPoint();

  kml.m_bookmarksData.emplace_back(CreateBookmark(start, true /* isStart */));
  kml.m_bookmarksData.emplace_back(CreateBookmark(finish, false /* isStart */));

  kml::TrackData mapsmeTrack;
  mapsmeTrack.m_points = mapsmeResult.GetRoutes().back().m_followedPolyline.GetPolyline().GetPoints();
  addTrack(std::move(mapsmeTrack));

  for (auto const & route : apiResult.GetRoutes())
  {
    kml::TrackData apiTrack;
    apiTrack.m_points = ConvertToPointDVector(route.GetWaypoints());
    addTrack(std::move(apiTrack));
  }

  kml::SerializerKml ser(kml);
  FileWriter sink(kmlFile);
  ser.Serialize(sink);
}
}  // namespace

namespace routing_quality
{
namespace routing_quality_tool
{
Result::Result(std::string mapsmeDumpPath, std::string anotherDumpPath,
               ms::LatLon const & start, ms::LatLon const & finish, double similarity,
               double etaDiffPercent)
  : m_mapsmeDumpPath(std::move(mapsmeDumpPath))
  , m_anotherDumpPath(std::move(anotherDumpPath))
  , m_start(start)
  , m_finish(finish)
  , m_similarity(similarity)
  , m_etaDiffPercent(etaDiffPercent) {}

bool Result::operator<(Result const & rhs) const
{
  return m_similarity < rhs.m_similarity;
}

RoutesSaver::RoutesSaver(std::string targetDir, ComparisonType comparisonType)
  : m_targetDir(std::move(targetDir))
  , m_comparisonType(comparisonType)
{
  OpenOutputStream();
}

void RoutesSaver::PushRoute(Result const & result)
{
  WriteStartAndFinish(m_output, result.m_start, result.m_finish);
  m_output << result.m_mapsmeDumpPath << " " << result.m_anotherDumpPath << std::endl;
}

void RoutesSaver::PushError(routing::RouterResultCode code,
                            ms::LatLon const & start,
                            ms::LatLon const & finish)
{
  CHECK_NOT_EQUAL(code, routing::RouterResultCode::NoError, ("Only errors codes."));

  auto & ofstream = m_errorRoutes[code];
  if (!ofstream.is_open())
  {
    std::string const fullpath =
        base::JoinPath(m_targetDir, DebugPrint(code) + ".routes");

    ofstream.open(fullpath);
    CHECK(ofstream.good(), ("Can not open:", fullpath, "for writing."));

    ofstream << std::setprecision(20);
    LOG(LINFO, ("Save routes with error:", code, "to:", fullpath));
  }

  WriteStartAndFinish(ofstream, start, finish);
}

void RoutesSaver::PushRivalError(ms::LatLon const & start, ms::LatLon const & finish)
{
  static std::string const kApiErrorsFile = "api_errors.routes";
  if (!m_apiErrors.is_open())
  {
    std::string const & fullpath = base::JoinPath(m_targetDir, kApiErrorsFile);
    m_apiErrors.open(fullpath);
    CHECK(m_apiErrors.good(), ("Can not open:", fullpath, "for writing."));
    m_apiErrors << std::setprecision(20);
    LOG(LINFO, ("Routes that api can not build, but mapsme do, placed:", fullpath));
  }

  WriteStartAndFinish(m_apiErrors, start, finish);
}

void RoutesSaver::TurnToNextFile()
{
  ++m_fileNumber;

  if (m_output.is_open())
    m_output.close();

  OpenOutputStream();
}

std::string RoutesSaver::GetCurrentPath() const
{
  std::string const fullpath =
      base::JoinPath(m_targetDir, std::to_string(m_fileNumber) + ".routes");

  return fullpath;
}

std::string const & RoutesSaver::GetTargetDir() const
{
  return m_targetDir;
}

ComparisonType RoutesSaver::GetComparsionType() const
{
  return m_comparisonType;
}

void RoutesSaver::OpenOutputStream()
{
  std::string const & fullpath = GetCurrentPath();
  m_output.open(fullpath);
  CHECK(m_output.good(), ("Can not open:", fullpath, "for writing."));
  m_output << std::setprecision(20);
}

void RoutesSaver::WriteStartAndFinish(std::ofstream & output,
                                      ms::LatLon const & start,
                                      ms::LatLon const & finish)
{
  output << start.m_lat << " " << start.m_lon << " "
         << finish.m_lat << " " << finish.m_lon
         << std::endl;
}

// This function creates python script that shows the distribution error.
void CreatePythonScriptForDistribution(std::string const & pythonScriptPath,
                                       std::string const & title,
                                       std::vector<double> const & values)
{
  std::ofstream python(pythonScriptPath);
  CHECK(python.good(), ("Can not open:", pythonScriptPath, "for writing."));

  std::string pythonArray = "[";
  for (auto const & value : values)
    pythonArray += std::to_string(value) + ",";

  if (values.empty())
    pythonArray += "]";
  else
    pythonArray.back() = ']';

  python << R"(
import numpy as np
import matplotlib.pyplot as plt

a = np.hstack()" + pythonArray + R"()
plt.hist(a, bins='auto')  # arguments are passed to np.histogram
plt.title(")" + title + R"(")
plt.show()
)";

  LOG(LINFO, ("Run: python", pythonScriptPath, "to look at:", title));
}

void CreatePythonGraphByPointsXY(std::string const & pythonScriptPath,
                                 std::string const & xlabel,
                                 std::string const & ylabel,
                                 std::vector<m2::PointD> const & points)
{
  std::ofstream python(pythonScriptPath);
  CHECK(python.good(), ("Can not open:", pythonScriptPath, "for writing."));

  std::string pythonArrayX = "[";
  std::string pythonArrayY = "[";
  for (auto const & point : points)
  {
    pythonArrayX += std::to_string(point.x) + ",";
    pythonArrayY += std::to_string(point.y) + ",";
  }

  if (points.empty())
  {
    pythonArrayX += "]";
    pythonArrayY += "]";
  }
  else
  {
    pythonArrayX.back() = ']';
    pythonArrayY.back() = ']';
  }

  python << R"(
import pylab

xlist = )" + pythonArrayX + R"(
ylist = )" + pythonArrayY + R"(

pylab.plot (xlist, ylist)
pylab.xlabel(")" + xlabel + R"(")
pylab.ylabel(")" + ylabel + R"(")
pylab.show()
)";

  LOG(LINFO, ("Run: python", pythonScriptPath, "to look at:", ylabel, "versus", xlabel));
}

void CreatePythonBarByMap(std::string const & pythonScriptPath,
                          std::map<std::string, size_t> const & stat,
                          std::string const & xlabel,
                          std::string const & ylabel)
{
  std::ofstream python(pythonScriptPath);
  CHECK(python.good(), ("Can not open:", pythonScriptPath, "for writing."));

  std::string labels = "[";
  std::string counts = "[";
  for (auto const & item : stat)
  {
    auto const & label = item.first;
    auto const & count = item.second;

    labels += "'" + label + "'" + ",";
    counts += std::to_string(count) + ",";
  }

  if (stat.empty())
  {
    labels += "]";
    counts += "]";
  }
  else
  {
    labels.back() = ']';
    counts.back() = ']';
  }

  python << R"(
import matplotlib
import matplotlib.pyplot as plt

labels = )" + labels + R"(
counts = )" + counts + R"(

summ = 0
for count in counts:
    summ += count

x = range(len(labels))  # the label locations
width = 0.35  # the width of the bars

fig, ax = plt.subplots()
rects = ax.bar(x, counts, width)

ax.set_ylabel(')" + ylabel + R"(')
ax.set_title(')" + xlabel + R"(')
ax.set_xticks(x)
ax.set_xticklabels(labels)
ax.legend()

def autolabel(rects):
    for rect in rects:
        height = rect.get_height()
        ax.annotate('{} ({}%)'.format(height, height / summ * 100),
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')

autolabel(rects)
fig.tight_layout()
plt.show()
)";

  LOG(LINFO, ("Run: python", pythonScriptPath, "to look at bar:", ylabel, "versus", xlabel));
}

/// \brief |SimilarityCounter| groups routes that we compare by similarity, here we tune these groups.
// static
std::vector<SimilarityCounter::Interval> const SimilarityCounter::kIntervals  = {
    {"[0.0, 0.0]", 0, 0 + 1e-5},
    {"[0.0, 0.1)", 0, 0.1},
    {"[0.1, 0.2)", 0.1, 0.2},
    {"[0.2, 0.3)", 0.2, 0.3},
    {"[0.3, 0.6)", 0.3, 0.6},
    {"[0.6, 0.8)", 0.6, 0.8},
    {"[0.8, 1.0)", 0.8, 1.0},
    {"[1.0, 1.0]", 1.0, 1.0 + 1e-5},
};

SimilarityCounter::SimilarityCounter(RoutesSaver & routesSaver)
  : m_routesSaver(routesSaver)
{
  for (auto const & interval : kIntervals)
    m_routesCounter.emplace_back(interval.m_name, 0);
}

SimilarityCounter::~SimilarityCounter()
{
  size_t maxNumberLength = 0;
  size_t maxKeyLength = 0;
  size_t sum = 0;
  for (auto const & item : m_routesCounter)
  {
    sum += item.m_routesNumber;
    maxNumberLength = std::max(maxNumberLength, std::to_string(item.m_routesNumber).size());
    maxKeyLength = std::max(maxKeyLength, item.m_intervalName.size());
  }

  for (auto const & item : m_routesCounter)
  {
    auto const percent = static_cast<double>(item.m_routesNumber) / sum * 100.0;
    auto const percentStr = MakeStringFromPercent(percent);

    PrintWithSpaces(item.m_intervalName, maxKeyLength + 1);
    std::cout << " => ";
    PrintWithSpaces(std::to_string(item.m_routesNumber), maxNumberLength + 1);
    std::cout << "( " << percentStr << " )" << std::endl;
  }
}

void SimilarityCounter::Push(Result const & result)
{
  auto const left = kIntervals[m_currentInterval].m_left;
  auto const right = kIntervals[m_currentInterval].m_right;
  if (!(left <= result.m_similarity && result.m_similarity < right))
  {
    ++m_currentInterval;
    m_routesSaver.TurnToNextFile();
  }

  if (m_routesCounter[m_currentInterval].m_routesNumber == 0)
  {
    LOG(LINFO,
        ("Save routes with:", m_routesCounter[m_currentInterval].m_intervalName,
         "similarity to:", m_routesSaver.GetCurrentPath()));
  }

  CHECK_LESS(m_currentInterval, m_routesCounter.size(), ());
  ++m_routesCounter[m_currentInterval].m_routesNumber;
  m_routesSaver.PushRoute(result);
}

void SimilarityCounter::CreateKmlFiles(double percent, std::vector<Result> const & results)
{
  size_t realResultIndex = 0;
  for (size_t intervalId = 0; intervalId < m_routesCounter.size(); ++intervalId)
  {
    std::string savePath =
        base::JoinPath(m_routesSaver.GetTargetDir(), std::to_string(intervalId));

    auto const currentSize = m_routesCounter[intervalId].m_routesNumber;
    auto const resultSize = static_cast<size_t>(currentSize * percent / 100.0);
    if (resultSize == 0)
      continue;

    auto const mkdirRes = Platform::MkDir(savePath);
    CHECK(mkdirRes == Platform::EError::ERR_OK ||
          mkdirRes == Platform::EError::ERR_FILE_ALREADY_EXISTS,
          ("Cannot create dir:", savePath));

    LOG(LINFO, ("Saving", resultSize,
                "kmls for:", m_routesCounter[intervalId].m_intervalName,
                "to:", savePath));

    auto const step = static_cast<size_t>(currentSize / resultSize);

    for (size_t i = 0; i < currentSize; ++i)
    {
      if (i % step != 0)
      {
        ++realResultIndex;
        continue;
      }

      CHECK_LESS(realResultIndex, results.size(), ());
      auto const mapsmeResult = RoutesBuilder::Result::Load(results[realResultIndex].m_mapsmeDumpPath);

      if (m_routesSaver.GetComparsionType() == ComparisonType::MapsmeVsApi)
      {
        auto const apiResult = api::Response::Load(results[realResultIndex].m_anotherDumpPath);
        std::string const kmlFile = base::JoinPath(savePath, std::to_string(i) + ".kml");
        SaveKmlFileDataTo(mapsmeResult, apiResult, kmlFile);
      }
      else
      {
        auto const mapsmeAnotherResult = RoutesBuilder::Result::Load(results[realResultIndex].m_anotherDumpPath);
        std::string const kmlFile = base::JoinPath(savePath, std::to_string(i) + ".kml");
        SaveKmlFileDataTo(mapsmeResult, mapsmeAnotherResult, kmlFile);
      }

      ++realResultIndex;
    }
  }
}
}  // namespace routing_quality_tool
}  // namespace routing_quality
