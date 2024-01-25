#include "map/framework.hpp"

#include "software_renderer/cpu_drawer.hpp"
#include "software_renderer/feature_processor.hpp"
#include "software_renderer/frame_image.hpp"

#include "drape_frontend/visual_params.hpp"

#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include <gflags/gflags.h>

#pragma mark Define options
//----------------------------------------------------------------------------------------
DEFINE_bool(c, false, "Read places from stdin");
DEFINE_string(place, "", "Define place in format \"lat;lon;zoom\"");
DEFINE_string(outpath, "./", "Path for output files");
DEFINE_string(datapath, "", "Path to data directory");
DEFINE_string(mwmpath, "", "Path to mwm files");
DEFINE_int32(width, 480, "Resulting image width");
DEFINE_int32(height, 640, "Resulting image height");
DEFINE_double(vs, 2.0, "Visual scale (mdpi = 1.0, hdpi = 1.5, xhdpiScale = 2.0, "
                       "6plus = 2.4, xxhdpi = 3.0, xxxhdpi = 3.5)");

//----------------------------------------------------------------------------------------

using namespace std;

namespace
{
struct Place
{
  double lat;
  double lon;
  int zoom;
  int width;
  int height;
};

Place ParsePlace(string const & src)
{
  Place p;
  try
  {
    strings::SimpleTokenizer token(src, ";");
    p.lat = stod(*token);
    p.lon = stod(*(++token));
    p.zoom = static_cast<int>(stoi(*(++token)));
  }
  catch (exception & e)
  {
    cerr << "Error in [" << src << "]: " << e.what() << endl;
    exit(1);
  }
  return p;
}

string FilenameSeq(string const & path)
{
  static size_t counter = 0;
  stringstream filename;
  filename << path << "mapshot" << setw(6) << setfill('0') << counter++ << ".png";
  return filename.str();
}

unique_ptr<software_renderer::CPUDrawer> cpuDrawer;

bool IsFrameRendererInitialized()
{
  return cpuDrawer != nullptr;
}

void InitFrameRenderer(float visualScale)
{
  using namespace software_renderer;

  if (cpuDrawer == nullptr)
  {
    df::VisualParams::Init(visualScale, 1024 /* dummy tile size */);
    string resPostfix = df::VisualParams::GetResourcePostfix(visualScale);
    cpuDrawer = make_unique_dp<CPUDrawer>(CPUDrawer::Params(resPostfix, visualScale));
  }
}

void ReleaseFrameRenderer()
{
  if (IsFrameRendererInitialized())
    cpuDrawer.reset();
}

/// @param center - map center in Mercator
/// @param zoomModifier - result zoom calculate like "base zoom" + zoomModifier
///                       if we are have search result "base zoom" calculate that my position and search result
///                       will be see with some bottom clamp.
///                       if we are't have search result "base zoom" == scales::GetUpperComfortScale() - 1
/// @param pxWidth - result image width.
///                  It must be equal render buffer width. For retina it's equal 2.0 * displayWidth
/// @param pxHeight - result image height.
///                   It must be equal render buffer height. For retina it's equal 2.0 * displayHeight
/// @param symbols - configuration for symbols on the frame
/// @param image [out] - result image
void DrawFrame(Framework & framework,
               m2::PointD const & center, int zoomModifier,
               uint32_t pxWidth, uint32_t pxHeight,
               software_renderer::FrameSymbols const & symbols,
               software_renderer::FrameImage & image)
{
  ASSERT(IsFrameRendererInitialized(), ());

  int resultZoom = -1;
  ScreenBase screen = cpuDrawer->CalculateScreen(center, zoomModifier, pxWidth, pxHeight, symbols, resultZoom);
  ASSERT_GREATER(resultZoom, 0, ());

  uint32_t const bgColor = drule::rules().GetBgColor(resultZoom);
  cpuDrawer->BeginFrame(pxWidth, pxHeight, dp::Extract(bgColor, 255 - (bgColor >> 24)));

  m2::RectD renderRect = m2::RectD(0, 0, pxWidth, pxHeight);
  m2::RectD selectRect;
  m2::RectD clipRect;
  double const inflationSize = 24 * cpuDrawer->GetVisualScale();
  screen.PtoG(m2::Inflate(renderRect, inflationSize, inflationSize), clipRect);
  screen.PtoG(renderRect, selectRect);

  uint32_t const tileSize = static_cast<uint32_t>(df::CalculateTileSize(pxWidth, pxHeight));
  int const drawScale = df::GetDrawTileScale(screen, tileSize, cpuDrawer->GetVisualScale());
  software_renderer::FeatureProcessor doDraw(make_ref(cpuDrawer), clipRect, screen, drawScale);

  int const upperScale = scales::GetUpperScale();

  framework.GetDataSource().ForEachInRect([&doDraw](FeatureType & ft) { doDraw(ft); },
                                          selectRect, min(upperScale, drawScale));

  cpuDrawer->Flush();
  //cpuDrawer->DrawMyPosition(screen.GtoP(center));

  if (symbols.m_showSearchResult)
  {
    if (!screen.PixelRect().IsPointInside(screen.GtoP(symbols.m_searchResult)))
      cpuDrawer->DrawSearchArrow(ang::AngleTo(center, symbols.m_searchResult));
    else
      cpuDrawer->DrawSearchResult(screen.GtoP(symbols.m_searchResult));
  }

  cpuDrawer->EndFrame(image);
}

void RenderPlace(Framework & framework, Place const & place, string const & filename)
{
  software_renderer::FrameImage frame;
  software_renderer::FrameSymbols sym;
  sym.m_showSearchResult = false;

  // If you are interested why, look at CPUDrawer::CalculateScreen.
  // It is almost UpperComfortScale but there is some magic involved.
  int constexpr kMagicBaseScale = 17;

  DrawFrame(framework, mercator::FromLatLon(place.lat, place.lon),
            place.zoom - kMagicBaseScale, place.width, place.height, sym, frame);

  ofstream file(filename.c_str());
  file.write(reinterpret_cast<char const *>(frame.m_data.data()), frame.m_data.size());
  file.close();
}
}  // namespace

int main(int argc, char * argv[])
{
  gflags::SetUsageMessage(
      "Generate screenshots of OMaps maps in chosen places, specified by coordinates and zoom.");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (!FLAGS_c && FLAGS_place.empty())
  {
    cerr << "Either -c or -place must be set" << endl;
    return 1;
  }

  if (!FLAGS_datapath.empty())
    GetPlatform().SetResourceDir(FLAGS_datapath);

  if (!FLAGS_mwmpath.empty())
    GetPlatform().SetWritableDirForTests(FLAGS_mwmpath);

  try
  {
    Framework f(FrameworkParams(false /* m_enableDiffs */));

    auto processPlace = [&](string const & place)
    {
      Place p = ParsePlace(place);
      p.width = FLAGS_width;
      p.height = FLAGS_height;
      string const & filename = FilenameSeq(FLAGS_outpath);
      RenderPlace(f, p, filename);
      cout << "Rendering " << place << " into " << filename << " is finished." << endl;
    };

    InitFrameRenderer(FLAGS_vs);

    if (!FLAGS_place.empty())
      processPlace(FLAGS_place);

    if (FLAGS_c)
    {
      for (string line; getline(cin, line);)
        processPlace(line);
    }

    ReleaseFrameRenderer();
    return 0;
  }
  catch (exception & e)
  {
    ReleaseFrameRenderer();
    cerr << e.what() << endl;
  }
  return 1;
}
