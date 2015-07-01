#include "map/benchmark_engine.hpp"
#include "map/framework.hpp"

#include "indexer/data_header.hpp"

#include "platform/settings.hpp"
#include "platform/platform.hpp"

#include "coding/file_container.hpp"
#include "coding/reader_streambuf.hpp"

#include "std/fstream.hpp"


class DoGetBenchmarks
{
  set<string> m_processed;

  BenchmarkEngine & m_engine;
  Platform & m_pl;

public:
  DoGetBenchmarks(BenchmarkEngine & engine)
    : m_engine(engine), m_pl(GetPlatform())
  {
  }

  void operator() (vector<string> const & v)
  {
    if (v[0][0] == '#')
      return;

    BenchmarkEngine::Benchmark b;
    b.m_name = v[1];

    m2::RectD r;
    if (m_processed.insert(v[0]).second)
    {
      try
      {
        feature::DataHeader header;
        header.Load(FilesContainerR(m_pl.GetReader(v[0])).GetReader(HEADER_FILE_TAG));
        r = header.GetBounds();
      }
      catch (RootException const & e)
      {
        LOG(LINFO, ("Can't add", v[0], "file to benchmark:", e.Msg()));
        return;
      }
    }

    int lastScale;
    if (v.size() > 3)
    {
      double lat1, lon1, lat2, lon2;
      CHECK(strings::to_double(v[2], lat1), (v[2]));
      CHECK(strings::to_double(v[3], lon1), (v[3]));
      CHECK(strings::to_double(v[4], lat2), (v[4]));
      CHECK(strings::to_double(v[5], lon2), (v[5]));

      r = m2::RectD(m2::PointD(MercatorBounds::LonToX(lon1), MercatorBounds::LatToY(lat1)),
                    m2::PointD(MercatorBounds::LonToX(lon2), MercatorBounds::LatToY(lat2)));
      CHECK(strings::to_int(v[6], lastScale), (v[6]));
    }
    else
      CHECK(strings::to_int(v[2], lastScale), (v[2]));

    ASSERT ( r != m2::RectD::GetEmptyRect(), (r) );

    Navigator & nav = m_engine.m_framework->GetNavigator();

    nav.SetFromRect(m2::AnyRectD(r));
    r = nav.Screen().GlobalRect().GetGlobalRect();

    b.m_provider.reset(new BenchmarkRectProvider(nav.GetDrawScale(), r, lastScale));

    m_engine.m_benchmarks.push_back(b);
  }
};

template <class ToDo>
void ForEachBenchmarkRecord(ToDo & toDo)
{
  try
  {
    string configPath;
    if (!Settings::Get("BenchmarkConfig", configPath))
      return;

    ReaderStreamBuf buffer(GetPlatform().GetReader(configPath, "w"));
    istream stream(&buffer);

    string line;
    while (stream.good())
    {
      getline(stream, line);

      vector<string> parts;
      strings::SimpleTokenizer it(line, " \t");
      while (it)
      {
        parts.push_back(*it);
        ++it;
      }

      if (!parts.empty())
        toDo(parts);
    }
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Error reading benchmarks:", e.Msg()));
  }
}

struct MapsCollector
{
  vector<string> m_maps;

  void operator() (vector<string> const & v)
  {
    ASSERT(!v[0].empty(), ());
    if (v[0][0] != '#')
      m_maps.push_back(v[0]);
  }
};

void BenchmarkEngine::PrepareMaps()
{
  // Deregister all previously registered maps in framework constructor.
  m_framework->DeregisterAllMaps();

  // add only maps needed for benchmarks
  MapsCollector collector;
  ForEachBenchmarkRecord(collector);
  for_each(collector.m_maps.begin(), collector.m_maps.end(),
           bind(&Framework::RegisterMap, m_framework, _1));
}

BenchmarkEngine::BenchmarkEngine(Framework * fw)
  : m_paintDuration(0),
    m_maxDuration(0),
    m_framework(fw)
{
}

void BenchmarkEngine::BenchmarkCommandFinished()
{
  double duration = m_paintDuration;

  if (duration > m_maxDuration)
  {
    m_maxDuration = duration;
    m_maxDurationRect = m_curBenchmarkRect;
  }

  BenchmarkResult res;
  res.m_name = m_benchmarks[m_curBenchmark].m_name;
  res.m_rect = m_curBenchmarkRect;
  res.m_time = duration;
  m_benchmarkResults.push_back(res);

  if (m_benchmarkResults.size() > 100)
    SaveBenchmarkResults();

  m_paintDuration = 0;
}

void BenchmarkEngine::SaveBenchmarkResults()
{
  string resultsPath;
  Settings::Get("BenchmarkResults", resultsPath);

  ofstream fout(GetPlatform().WritablePathForFile(resultsPath).c_str(), ios::app);
  for (size_t i = 0; i < m_benchmarkResults.size(); ++i)
  {
    /// @todo Place correct version here from bundle (platform).

    fout << GetPlatform().DeviceName() << " "
         << "3.0.0" << " "
         << m_startTime << " "
         << m_benchmarkResults[i].m_name << " "
         << m_benchmarkResults[i].m_rect.minX() << " "
         << m_benchmarkResults[i].m_rect.minY() << " "
         << m_benchmarkResults[i].m_rect.maxX() << " "
         << m_benchmarkResults[i].m_rect.maxY() << " "
         << m_benchmarkResults[i].m_time << endl;
  }

  m_benchmarkResults.clear();
}

void BenchmarkEngine::SendBenchmarkResults()
{
}

void BenchmarkEngine::MarkBenchmarkResultsEnd()
{
  string resultsPath;
  Settings::Get("BenchmarkResults", resultsPath);
  ofstream fout(GetPlatform().WritablePathForFile(resultsPath).c_str(), ios::app);
  fout << "END " << m_startTime << endl;
}

void BenchmarkEngine::MarkBenchmarkResultsStart()
{
  string resultsPath;
  Settings::Get("BenchmarkResults", resultsPath);
  ofstream fout(GetPlatform().WritablePathForFile(resultsPath).c_str(), ios::app);
  fout << "START " << m_startTime << endl;
}

bool BenchmarkEngine::NextBenchmarkCommand()
{
#ifndef USE_DRAPE
  if (m_benchmarks[m_curBenchmark].m_provider->hasRect() || ++m_curBenchmark < m_benchmarks.size())
  {
    double const s = m_benchmarksTimer.ElapsedSeconds();

    int const fenceID = m_framework->GetRenderPolicy()->InsertBenchmarkFence();

    m_framework->ShowRect(m_benchmarks[m_curBenchmark].m_provider->nextRect());
    m_curBenchmarkRect = m_framework->GetCurrentViewport();

    m_framework->GetRenderPolicy()->JoinBenchmarkFence(fenceID);

    m_paintDuration += m_benchmarksTimer.ElapsedSeconds() - s;
    BenchmarkCommandFinished();

    return true;
  }
  else
  {
    SaveBenchmarkResults();
    MarkBenchmarkResultsEnd();
    SendBenchmarkResults();

    LOG(LINFO, ("Bechmarks took", m_benchmarksTimer.ElapsedSeconds(), "seconds to complete"));
    return false;
  }
#else
  return false;
#endif // USE_DRAPE
}

void BenchmarkEngine::Start()
{
  m_thread.Create(make_unique<Routine>(*this));
}

BenchmarkEngine::Routine::Routine(BenchmarkEngine & engine) : m_engine(engine) {}

void BenchmarkEngine::Routine::Do()
{
  m_engine.PrepareMaps();

  int benchMarkCount = 1;
  (void) Settings::Get("BenchmarkCyclesCount", benchMarkCount);

  for (int i = 0; i < benchMarkCount; ++i)
  {
    DoGetBenchmarks doGet(m_engine);
    ForEachBenchmarkRecord(doGet);

    m_engine.m_curBenchmark = 0;

    m_engine.m_benchmarksTimer.Reset();
    m_engine.m_startTime = my::FormatCurrentTime();

    m_engine.MarkBenchmarkResultsStart();
    while (m_engine.NextBenchmarkCommand()){};

    m_engine.m_benchmarks.clear();
  }
}
