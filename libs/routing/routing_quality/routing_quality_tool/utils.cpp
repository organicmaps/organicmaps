#include "routing/routing_quality/routing_quality_tool/utils.hpp"

#include "kml/serdes.hpp"
#include "kml/types.hpp"

#include "geometry/point_with_altitude.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <array>
#include <iomanip>
#include <utility>

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

std::vector<geometry::PointWithAltitude> ConvertToPointsWithAltitudes(std::vector<ms::LatLon> const & latlons)
{
  std::vector<geometry::PointWithAltitude> result;
  result.reserve(latlons.size());
  for (size_t i = 0; i < latlons.size(); ++i)
    result.emplace_back(mercator::FromLatLon(latlons[i]), geometry::kDefaultAltitudeMeters);

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
void SaveKmlFileDataTo(RoutesBuilder::Result const & mapsmeResult, AnotherResult const & apiResult,
                       std::string const & kmlFile)
{
  static std::array<uint32_t, 5> const kColors = {
      0xff0000ff,  // Red
      0x0000ffff,  // Blue
      0x00ff00ff,  // Green
      0xffa500ff,  // Orange
      0xa52a2aff   // Brown
  };

  kml::FileData kml;

  size_t colorNumber = 0;
  auto const addTrack = [&](kml::MultiGeometry::LineT && line)
  {
    kml::TrackData track;
    track.m_geometry.m_lines.push_back(std::move(line));

    CHECK_LESS(colorNumber, kColors.size(), ());
    track.m_layers = {{5.0 /* lineWidth */, {kml::PredefinedColor::None, kColors[colorNumber++]}}};
    kml.m_tracksData.emplace_back(std::move(track));
  };

  auto const & start = apiResult.GetStartPoint();
  auto const & finish = apiResult.GetFinishPoint();

  kml.m_bookmarksData.emplace_back(CreateBookmark(start, true /* isStart */));
  kml.m_bookmarksData.emplace_back(CreateBookmark(finish, false /* isStart */));

  auto const & resultPoints = mapsmeResult.GetRoutes().back().m_followedPolyline.GetPolyline().GetPoints();

  kml::MultiGeometry::LineT mmTrack;
  mmTrack.reserve(resultPoints.size());
  for (auto const & pt : resultPoints)
    mmTrack.emplace_back(pt, geometry::kDefaultAltitudeMeters);

  addTrack(std::move(mmTrack));

  for (auto const & route : apiResult.GetRoutes())
    addTrack(ConvertToPointsWithAltitudes(route.GetWaypoints()));

  kml::SerializerKml ser(kml);
  FileWriter sink(kmlFile);
  ser.Serialize(sink);
}

template <typename T>
std::string CreatePythonArray(std::vector<T> const & data, bool isString = false)
{
  std::stringstream ss;
  ss << "[";
  for (auto const & item : data)
  {
    if (isString)
      ss << "\"";
    ss << item;
    if (isString)
      ss << "\"";
    ss << ",";
  }

  auto result = ss.str();
  if (data.empty())
    result += "]";
  else
    result.back() = ']';

  return result;
}
}  // namespace

namespace routing_quality
{
namespace routing_quality_tool
{
Result::Result(std::string mapsmeDumpPath, std::string anotherDumpPath, ms::LatLon const & start,
               ms::LatLon const & finish, double similarity, double etaDiffPercent)
  : m_mapsmeDumpPath(std::move(mapsmeDumpPath))
  , m_anotherDumpPath(std::move(anotherDumpPath))
  , m_start(start)
  , m_finish(finish)
  , m_similarity(similarity)
  , m_etaDiffPercent(etaDiffPercent)
{}

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

void RoutesSaver::PushError(routing::RouterResultCode code, ms::LatLon const & start, ms::LatLon const & finish)
{
  CHECK_NOT_EQUAL(code, routing::RouterResultCode::NoError, ("Only errors codes."));

  auto & ofstream = m_errorRoutes[code];
  if (!ofstream.is_open())
  {
    std::string const fullpath = base::JoinPath(m_targetDir, DebugPrint(code) + ".routes");

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
  std::string const fullpath = base::JoinPath(m_targetDir, std::to_string(m_fileNumber) + ".routes");

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

void RoutesSaver::WriteStartAndFinish(std::ofstream & output, ms::LatLon const & start, ms::LatLon const & finish)
{
  output << start.m_lat << " " << start.m_lon << " " << finish.m_lat << " " << finish.m_lon << std::endl;
}

// This function creates python script that shows the distribution error.
void CreatePythonScriptForDistribution(std::string const & pythonScriptPath, std::string const & title,
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

a = np.hstack()" +
                pythonArray + R"()
plt.hist(a, bins='auto')  # arguments are passed to np.histogram
plt.title(")" + title +
                R"(")
plt.show()
)";

  LOG(LINFO, ("Run: python", pythonScriptPath, "to look at:", title));
}

void CreatePythonGraphByPointsXY(std::string const & pythonScriptPath, std::string const & xlabel,
                                 std::string const & ylabel, std::vector<std::vector<m2::PointD>> const & graphics,
                                 std::vector<std::string> const & legends)
{
  CHECK_EQUAL(legends.size(), graphics.size(), ());
  std::ofstream python(pythonScriptPath);
  CHECK(python.good(), ("Can not open:", pythonScriptPath, "for writing."));

  std::string pythonArrayX = "[";
  std::string pythonArrayY = "[";
  std::string legendsArray = "[";
  for (size_t i = 0; i < graphics.size(); ++i)
  {
    auto const & points = graphics[i];
    pythonArrayX += "[";
    pythonArrayY += "[";
    legendsArray += "\"" + legends[i] + "\",";
    for (auto const & point : points)
    {
      pythonArrayX += std::to_string(point.x) + ",";
      pythonArrayY += std::to_string(point.y) + ",";
    }

    pythonArrayX += "],";
    pythonArrayY += "],";
  }

  pythonArrayX.back() = ']';
  pythonArrayY.back() = ']';
  legendsArray.back() = ']';

  python << R"(
import pylab

legends = )" + legendsArray +
                R"(
xlist = )" + pythonArrayX +
                R"(
ylist = )" + pythonArrayY +
                R"(
for (x, y, l) in zip(xlist, ylist, legends):
  pylab.plot(x, y, label=l)

pylab.xlabel(")" +
                xlabel + R"(")
pylab.ylabel(")" +
                ylabel + R"(")
pylab.legend()
pylab.tight_layout()
pylab.show()
)";

  LOG(LINFO, ("Run: python", pythonScriptPath, "to look at:", ylabel, "versus", xlabel));
}

void CreatePythonBarByMap(std::string const & pythonScriptPath, std::vector<std::string> const & barLabels,
                          std::vector<std::vector<double>> const & barHeights, std::vector<std::string> const & legends,
                          std::string const & xlabel, std::string const & ylabel, bool drawPercents)
{
  std::ofstream python(pythonScriptPath);
  CHECK(python.good(), ("Can not open:", pythonScriptPath, "for writing."));

  std::string labelsArray = CreatePythonArray(barLabels, true /* isString */);
  std::string legendsArray = CreatePythonArray(legends, true /* isString */);
  std::string counts = "[";
  for (auto const & heights : barHeights)
    counts += CreatePythonArray(heights) + ",";

  if (barHeights.empty())
    counts += "]";
  else
    counts.back() = ']';

  std::string const formatString =
      drawPercents ? "f'{round(height, 2)}({round(height / summ * 100, 2)}%)'" : "f'{round(height, 2)}'";

  python << R"(
import matplotlib
import matplotlib.pyplot as plt
import numpy as np

bar_width = 0.35
labels = )" + labelsArray +
                R"(
legends = )" + legendsArray +
                R"(
counts = )" + counts +
                R"(

x = np.arange(len(labels))  # the label locations
width = 0.35  # the width of the bars

fig, ax = plt.subplots()
bars = []
for i in range(len(counts)):
    bar = ax.bar(x + i * bar_width, counts[i], bar_width, label=legends[i])
    bars.append(bar)

ax.set_ylabel(')" +
                ylabel + R"(')
ax.set_title(')" +
                xlabel + R"(')
pos = (bar_width * (len(counts) - 1)) / 2
ax.set_xticks(x + pos)
ax.set_xticklabels(labels)
ax.legend()

def autolabel(rects, counts_ith):
    summ = 0
    for count in counts_ith:
        summ += count

    for rect in rects:
        height = rect.get_height()
        ax.annotate()" +
                formatString + R"(,
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')

for i in range(len(counts)):
    autolabel(bars[i], counts[i])
fig.tight_layout()
plt.show()
)";

  LOG(LINFO, ("Run: python", pythonScriptPath, "to look at bar:", ylabel, "versus", xlabel));
}

/// \brief |SimilarityCounter| groups routes that we compare by similarity, here we tune these groups.
// static
std::vector<SimilarityCounter::Interval> const SimilarityCounter::kIntervals = {
    {"[0.0, 0.0]", 0, 0 + 1e-5}, {"[0.0, 0.1)", 0, 0.1},   {"[0.1, 0.2)", 0.1, 0.2}, {"[0.2, 0.3)", 0.2, 0.3},
    {"[0.3, 0.6)", 0.3, 0.6},    {"[0.6, 0.8)", 0.6, 0.8}, {"[0.8, 1.0)", 0.8, 1.0}, {"[1.0, 1.0]", 1.0, 1.0 + 1e-5},
};

SimilarityCounter::SimilarityCounter(RoutesSaver & routesSaver) : m_routesSaver(routesSaver)
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
  auto left = kIntervals[m_currentInterval].m_left;
  auto right = kIntervals[m_currentInterval].m_right;
  while (!(left <= result.m_similarity && result.m_similarity < right))
  {
    ++m_currentInterval;
    m_routesSaver.TurnToNextFile();
    CHECK_LESS(m_currentInterval, m_routesCounter.size(), ());
    left = kIntervals[m_currentInterval].m_left;
    right = kIntervals[m_currentInterval].m_right;
  }

  CHECK_LESS(m_currentInterval, m_routesCounter.size(), ());
  if (m_routesCounter[m_currentInterval].m_routesNumber == 0)
  {
    LOG(LINFO, ("Save routes with:", m_routesCounter[m_currentInterval].m_intervalName,
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
    std::string savePath = base::JoinPath(m_routesSaver.GetTargetDir(), std::to_string(intervalId));

    auto const currentSize = m_routesCounter[intervalId].m_routesNumber;
    auto const resultSize = static_cast<size_t>(currentSize * percent / 100.0);
    if (resultSize == 0)
      continue;

    auto const mkdirRes = Platform::MkDir(savePath);
    CHECK(mkdirRes == Platform::EError::ERR_OK || mkdirRes == Platform::EError::ERR_FILE_ALREADY_EXISTS,
          ("Cannot create dir:", savePath));

    LOG(LINFO, ("Saving", resultSize, "kmls for:", m_routesCounter[intervalId].m_intervalName, "to:", savePath));

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

      std::string const kmlFile = base::JoinPath(savePath, std::to_string(i) + ".kml");
      if (m_routesSaver.GetComparsionType() == ComparisonType::MapsmeVsApi)
      {
        auto const apiResult = api::Response::Load(results[realResultIndex].m_anotherDumpPath);
        SaveKmlFileDataTo(mapsmeResult, apiResult, kmlFile);
      }
      else
      {
        auto const mapsmeAnotherResult = RoutesBuilder::Result::Load(results[realResultIndex].m_anotherDumpPath);
        SaveKmlFileDataTo(mapsmeResult, mapsmeAnotherResult, kmlFile);
      }

      ++realResultIndex;
    }
  }
}
}  // namespace routing_quality_tool
}  // namespace routing_quality
