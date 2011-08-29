#include "../base/SRC_FIRST.hpp"

#include "benchmark_framework.hpp"
#include "benchmark_provider.hpp"
#include "feature_vec_model.hpp"

#include "../coding/file_container.hpp"

#include "../std/fstream.hpp"

#include "../platform/settings.hpp"

#include "../version/version.hpp"

#include "benchmark_tiling_render_policy_mt.hpp"
#include "render_policy_st.hpp"

template <class T> class DoGetBenchmarks
{
  set<string> m_processed;
  vector<T> & m_benchmarks;
  Platform & m_pl;

public:
  DoGetBenchmarks(vector<T> & benchmarks)
    : m_benchmarks(benchmarks), m_pl(GetPlatform())
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
    b.m_provider.reset(new BenchmarkRectProvider(scales::GetScaleLevel(r), r, lastScale));

    m_benchmarks.push_back(b);
  }
};

template <class ToDo>
void ForEachBenchmarkRecord(ToDo & toDo)
{
  Platform & pl = GetPlatform();

  string buffer;
  try
  {
    string configPath;
    Settings::Get("BenchmarkConfig", configPath);
    ReaderPtr<Reader>(pl.GetReader(configPath)).ReadAsString(buffer);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Error reading benchmarks: ", e.what()));
    return;
  }

  istringstream stream(buffer);

  string line;
  while (stream.good())
  {
    getline(stream, line);

    vector<string> parts;
    strings::SimpleTokenizer it(line, " ");
    while (it)
    {
      parts.push_back(*it);
      ++it;
    }

    if (!parts.empty())
      toDo(parts);
  }
}

class FirstReaderAdder : public ReadersAdder
{
  typedef ReadersAdder base_type;
public:
  FirstReaderAdder(maps_list_t & lst) : base_type(GetPlatform(), lst) {}
  void operator() (vector<string> const & v)
  {
    if (!v[0].empty())
      if (v[0][0] == '#')
        return;
    base_type::operator() (v[0]);
  }
};

template <typename TModel>
void BenchmarkFramework<TModel>::EnumLocalMaps(typename base_type::maps_list_t & filesList)
{
  FirstReaderAdder adder(filesList);
  ForEachBenchmarkRecord(adder);
}

template <typename TModel>
BenchmarkFramework<TModel>::BenchmarkFramework(shared_ptr<WindowHandle> const & wh,
                                               size_t bottomShift)
  : base_type(wh, bottomShift),
    m_paintDuration(0),
    m_maxDuration(0),
    m_isBenchmarkFinished(false),
    m_isBenchmarkInitialized(false)
{
  bool isBenchmarkingMT = false;
  Settings::Get("IsBenchmarkingMT", isBenchmarkingMT);

  if (isBenchmarkingMT)
    base_type::SetRenderPolicy(make_shared_ptr(new BenchmarkTilingRenderPolicyMT(wh, bind(&base_type::DrawModel, this, _1, _2, _3, _4, _5))));
  else
    base_type::SetRenderPolicy(make_shared_ptr(new RenderPolicyST(wh, bind(&base_type::DrawModel, this, _1, _2, _3, _4, _5))));

  m_startTime = my::FormatCurrentTime();

#ifdef DEBUG
  base_type::m_informationDisplay.enableGlobalRect(false);
#endif

  base_type::m_informationDisplay.enableBenchmarkInfo(true);
}

template <typename TModel>
void BenchmarkFramework<TModel>::BenchmarkCommandFinished()
{
  double duration = m_paintDuration;

  if (duration > m_maxDuration)
  {
    m_maxDuration = duration;
    m_maxDurationRect = m_curBenchmarkRect;
    base_type::m_informationDisplay.addBenchmarkInfo("maxDurationRect: ", m_maxDurationRect, m_maxDuration);
  }

  BenchmarkResult res;
  res.m_name = m_benchmarks[m_curBenchmark].m_name;
  res.m_rect = m_curBenchmarkRect;
  res.m_time = duration;
  m_benchmarkResults.push_back(res);

  if (m_benchmarkResults.size() > 100)
    SaveBenchmarkResults();


  m_paintDuration = 0;
  if (!m_isBenchmarkFinished)
    NextBenchmarkCommand();
}

template <typename TModel>
void BenchmarkFramework<TModel>::SaveBenchmarkResults()
{
  string resultsPath;
  Settings::Get("BenchmarkResults", resultsPath);
  LOG(LINFO, (resultsPath));
  ofstream fout(GetPlatform().WritablePathForFile(resultsPath).c_str(), ios::app);

  for (size_t i = 0; i < m_benchmarkResults.size(); ++i)
  {
    fout << GetPlatform().DeviceID() << " "
         << VERSION_STRING << " "
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

template <typename TModel>
void BenchmarkFramework<TModel>::SendBenchmarkResults()
{
//    ofstream fout(GetPlatform().WritablePathForFile("benchmarks/results.txt").c_str(), ios::app);
//    fout << "[COMPLETED]";
//    fout.close();
  /// send to server for adding to statistics graphics
  /// and delete results file
}

template <typename TModel>
void BenchmarkFramework<TModel>::MarkBenchmarkResultsEnd()
{
  string resultsPath;
  Settings::Get("BenchmarkResults", resultsPath);
  LOG(LINFO, (resultsPath));
  ofstream fout(GetPlatform().WritablePathForFile(resultsPath).c_str(), ios::app);
  fout << "END " << m_startTime << endl;
}

template <typename TModel>
void BenchmarkFramework<TModel>::MarkBenchmarkResultsStart()
{
  string resultsPath;
  Settings::Get("BenchmarkResults", resultsPath);
  LOG(LINFO, (resultsPath));
  ofstream fout(GetPlatform().WritablePathForFile(resultsPath).c_str(), ios::app);
  fout << "START " << m_startTime << endl;
}

template <typename TModel>
void BenchmarkFramework<TModel>::NextBenchmarkCommand()
{
  if ((m_benchmarks[m_curBenchmark].m_provider->hasRect()) || (++m_curBenchmark < m_benchmarks.size()))
  {
    m_curBenchmarkRect = m_benchmarks[m_curBenchmark].m_provider->nextRect();
    base_type::m_navigator.SetFromRect(m_curBenchmarkRect);
    base_type::Invalidate();
  }
  else
  {
    static bool isFirstTime = true;
    if (isFirstTime)
    {
      m_isBenchmarkFinished = true;
      isFirstTime = false;
      SaveBenchmarkResults();
      MarkBenchmarkResultsEnd();
      SendBenchmarkResults();
      LOG(LINFO, ("Bechmarks took ", m_benchmarksTimer.ElapsedSeconds(), " seconds to complete"));
    }
  }
}

template <typename TModel>
void BenchmarkFramework<TModel>::InitBenchmark()
{
  DoGetBenchmarks<Benchmark> doGet(m_benchmarks);
  ForEachBenchmarkRecord(doGet);

  m_curBenchmark = 0;

  //base_type::m_renderPolicy->addAfterFrame(bind(&this_type::BenchmarkCommandFinished, this));

  m_benchmarksTimer.Reset();

  MarkBenchmarkResultsStart();
  NextBenchmarkCommand();

  base_type::Invalidate();
}

template <typename TModel>
void BenchmarkFramework<TModel>::OnSize(int w, int h)
{
  Framework<TModel>::OnSize(w, h);
  if (!m_isBenchmarkInitialized)
  {
    m_isBenchmarkInitialized = true;
    InitBenchmark();
  }
}

template <typename TModel>
void BenchmarkFramework<TModel>::Paint(shared_ptr<PaintEvent> e)
{
/*  m2::PointD const center = base_type::m_renderQueue.renderState().m_actualScreen.ClipRect().Center();
  base_type::m_informationDisplay.setScreen(m_renderQueue.renderState().m_actualScreen);
  base_type::m_informationDisplay.setCenter(m2::PointD(MercatorBounds::XToLon(center.x), MercatorBounds::YToLat(center.y)));

  if (!m_isBenchmarkInitialized)
  {
    e->drawer()->screen()->beginFrame();
    e->drawer()->screen()->clear(base_type::m_renderPolicy->bgColor());
    base_type::m_informationDisplay.setDisplayRect(m2::RectI(0, 0, 100, 100));
    base_type::m_informationDisplay.enableRuler(false);
    base_type::m_informationDisplay.doDraw(e->drawer().get());
    e->drawer()->screen()->endFrame();
  }
  else*/
  double s = m_benchmarksTimer.ElapsedSeconds();
  Framework<TModel>::Paint(e);
  m_paintDuration += m_benchmarksTimer.ElapsedSeconds() - s;
  if (!m_isBenchmarkFinished)
    BenchmarkCommandFinished();
}

template class BenchmarkFramework<model::FeaturesFetcher>;
