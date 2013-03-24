#include "benchmark_engine.hpp"
#include "benchmark_provider.hpp"

#include "../platform/settings.hpp"
#include "../platform/platform.hpp"

#include "../coding/file_container.hpp"
#include "../coding/reader_streambuf.hpp"

#include "../std/fstream.hpp"


template <class T> class DoGetBenchmarks
{
  set<string> m_processed;
  vector<T> & m_benchmarks;
  Navigator & m_navigator;
  Platform & m_pl;

public:
  DoGetBenchmarks(vector<T> & benchmarks, Navigator & navigator)
    : m_benchmarks(benchmarks), m_navigator(navigator), m_pl(GetPlatform())
  {
  }

  void operator() (vector<string> const & v)
  {
    if (v[0][0] == '#')
      return;

    T b;
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
        LOG(LINFO, ("Cannot add ", v[0], " file to benchmark: ", e.what()));
        return;
      }
    }

    int lastScale;
    if (v.size() > 3)
    {
      double x0, y0, x1, y1;
      strings::to_double(v[2], x0);
      strings::to_double(v[3], y0);
      strings::to_double(v[4], x1);
      strings::to_double(v[5], y1);
      r = m2::RectD(x0, y0, x1, y1);
      strings::to_int(v[6], lastScale);
    }
    else
      strings::to_int(v[2], lastScale);

    ASSERT ( r != m2::RectD::GetEmptyRect(), (r) );

    m_navigator.SetFromRect(m2::AnyRectD(r));
    r = m_navigator.Screen().GlobalRect().GetGlobalRect();

    b.m_provider.reset(new BenchmarkRectProvider(scales::GetScaleLevel(r), r, lastScale));

    m_benchmarks.push_back(b);
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

    ReaderStreamBuf buffer(GetPlatform().GetReader(configPath));
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
    LOG(LERROR, ("Error reading benchmarks: ", e.what()));
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
  // remove all previously added maps in framework constructor
  Platform::FilesList files;
  m_framework->GetLocalMaps(files);
  for_each(files.begin(), files.end(),
           bind(&Framework::RemoveMap, m_framework, _1));

  // add only maps needed for benchmarks
  MapsCollector collector;
  ForEachBenchmarkRecord(collector);
  for_each(collector.m_maps.begin(), collector.m_maps.end(),
           bind(&Framework::AddMap, m_framework, _1));
}

BenchmarkEngine::BenchmarkEngine(Framework * fw)
  : m_paintDuration(0),
    m_maxDuration(0),
    m_framework(fw)
{
//  m_framework->GetInformationDisplay().enableBenchmarkInfo(true);
}

void BenchmarkEngine::BenchmarkCommandFinished()
{
  double duration = m_paintDuration;

  if (duration > m_maxDuration)
  {
    m_maxDuration = duration;
    m_maxDurationRect = m_curBenchmarkRect;
    m_framework->GetInformationDisplay().addBenchmarkInfo("maxDurationRect: ", m_maxDurationRect, m_maxDuration);
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
         << "2.3.0" << " "
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
//    ofstream fout(GetPlatform().WritablePathForFile("benchmarks/results.txt").c_str(), ios::app);
//    fout << "[COMPLETED]";
//    fout.close();
  /// send to server for adding to statistics graphics
  /// and delete results file
}

void BenchmarkEngine::MarkBenchmarkResultsEnd()
{
  string resultsPath;
  Settings::Get("BenchmarkResults", resultsPath);
  LOG(LINFO, (resultsPath));
  ofstream fout(GetPlatform().WritablePathForFile(resultsPath).c_str(), ios::app);
  fout << "END " << m_startTime << endl;
}

void BenchmarkEngine::MarkBenchmarkResultsStart()
{
  string resultsPath;
  Settings::Get("BenchmarkResults", resultsPath);
  LOG(LINFO, (resultsPath));
  ofstream fout(GetPlatform().WritablePathForFile(resultsPath).c_str(), ios::app);
  fout << "START " << m_startTime << endl;
}

bool BenchmarkEngine::NextBenchmarkCommand()
{
  if ((m_benchmarks[m_curBenchmark].m_provider->hasRect()) || (++m_curBenchmark < m_benchmarks.size()))
  {
    double s = m_benchmarksTimer.ElapsedSeconds();

    int fenceID = m_framework->GetRenderPolicy()->InsertBenchmarkFence();

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
    LOG(LINFO, ("Bechmarks took ", m_benchmarksTimer.ElapsedSeconds(), " seconds to complete"));
    return false;
  }
}

void BenchmarkEngine::Start()
{
  m_thread.Create(this);
}

void BenchmarkEngine::Do()
{
  PrepareMaps();

  int benchMarkCount = 1;
  Settings::Get("BenchmarkCyclesCount", benchMarkCount);

  for (int i = 0; i < benchMarkCount; ++i)
  {
    DoGetBenchmarks<Benchmark> doGet(m_benchmarks, m_framework->m_navigator);
    ForEachBenchmarkRecord(doGet);

    m_curBenchmark = 0;

    m_benchmarksTimer.Reset();
    m_startTime = my::FormatCurrentTime();

    MarkBenchmarkResultsStart();
    while (NextBenchmarkCommand()){};

    m_benchmarks.clear();
  }
}
