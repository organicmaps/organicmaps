#include "dev_sandbox/context_factory.hpp"
#include "dev_sandbox/imgui_renderer.hpp"

#include "map/framework.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"
#include "platform/settings.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"

#include "std/target_os.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <clocale>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <gflags/gflags.h>

#include <vulkan_wrapper.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/imgui.h>

DEFINE_string(data_path, "", "Path to data directory.");
DEFINE_string(log_abort_level, base::ToString(base::GetDefaultLogAbortLevel()),
              "Log messages severity that causes termination.");
DEFINE_string(resources_path, "", "Path to resources directory.");
DEFINE_string(lang, "", "Preferred language override.");
#if defined(OMIM_OS_LINUX)
DEFINE_bool(smoke_test, false, "Render both APIs, exercise window transitions and exit (requires a display).");
#endif

namespace
{
bool ValidateLogAbortLevel(char const * flagname, std::string const & value)
{
  if (auto level = base::FromString(value); !level)
  {
    std::cerr << "Invalid value for --" << flagname << ": " << value << ", must be one of: ";
    auto const & names = base::GetLogLevelNames();
    for (size_t i = 0; i < names.size(); ++i)
    {
      if (i != 0)
        std::cerr << ", ";
      std::cerr << names[i];
    }
    std::cerr << '\n';
    return false;
  }
  return true;
}

bool const g_logAbortLevelDummy = gflags::RegisterFlagValidator(&FLAGS_log_abort_level, &ValidateLogAbortLevel);

void errorCallback(int error, char const * description)
{
  LOG(LERROR, ("GLFW (", error, "):", description));
}

struct WindowHandlers
{
  std::function<void(int w, int h)> onResize;
  std::function<void()> onIconify;
  std::function<void(double x, double y, int button, int action, int mods)> onMouseButton;
  std::function<void(double x, double y)> onMouseMove;
  std::function<void(double x, double y, double xOffset, double yOffset)> onScroll;
  std::function<void(int key, int scancode, int action, int mods)> onKeyboardButton;
  std::function<void(float xscale, float yscale)> onContentScale;
} handlers;

df::Touch GetTouch(double x, double y)
{
  return df::Touch{.m_location = m2::PointF(static_cast<float>(x), static_cast<float>(y)), .m_id = 0};
}

df::Touch GetSymmetricalTouch(Framework & framework, df::Touch const & touch)
{
  m2::PointD const pixelCenter = framework.GetVisiblePixelCenter();
  m2::PointD const symmetricalLocation = pixelCenter + pixelCenter - m2::PointD(touch.m_location);

  df::Touch result;
  result.m_id = touch.m_id + 1;
  result.m_location = symmetricalLocation;

  return result;
}

df::TouchEvent GetTouchEvent(Framework & framework, double x, double y, int mods, df::TouchEvent::ETouchType type)
{
  df::TouchEvent event;
  event.SetTouchType(type);
  event.SetFirstTouch(GetTouch(x, y));
  if (mods & GLFW_MOD_SUPER)
    event.SetSecondTouch(GetSymmetricalTouch(framework, event.GetFirstTouch()));
  return event;
}

void FormatMapSize(uint64_t sizeInBytes, std::string & units, size_t & sizeToDownload)
{
  int const mbInBytes = 1024 * 1024;
  int const kbInBytes = 1024;
  if (sizeInBytes > mbInBytes)
  {
    sizeToDownload = (sizeInBytes + mbInBytes - 1) / mbInBytes;
    units = "MB";
  }
  else if (sizeInBytes > kbInBytes)
  {
    sizeToDownload = (sizeInBytes + kbInBytes - 1) / kbInBytes;
    units = "KB";
  }
  else
  {
    sizeToDownload = sizeInBytes;
    units = "B";
  }
}

std::string_view GetMyPoisitionText(location::EMyPositionMode mode)
{
  switch (mode)
  {
  case location::EMyPositionMode::PendingPosition: return "Pending";
  case location::EMyPositionMode::NotFollowNoPosition: return "No position";
  case location::EMyPositionMode::NotFollow: return "Not follow";
  case location::EMyPositionMode::Follow: return "Follow";
  case location::EMyPositionMode::FollowAndRotate: return "Follow and Rotate";
  }
  return "";
}

dp::ApiVersion GetApiVersion(char const * apiLabel)
{
  std::string_view v(apiLabel);
  if (v == "Metal")
    return dp::ApiVersion::Metal;
  if (v == "Vulkan")
    return dp::ApiVersion::Vulkan;
  if (v == "OpenGL")
    return dp::ApiVersion::OpenGLES3;
  return dp::ApiVersion::Invalid;
}

#if defined(OMIM_OS_LINUX)
class LinuxGuiThread : public base::TaskLoop
{
public:
  PushResult Push(Task && task) override
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks.emplace_back(std::move(task));
    return {true, base::TaskLoop::kNoId};
  }

  PushResult Push(Task const & task) override { return Push(Task(task)); }

  void ExecuteTasks()
  {
    std::vector<Task> tasks;
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      tasks.swap(m_tasks);
    }
    for (auto & task : tasks)
      task();
  }

private:
  std::vector<Task> m_tasks;
  std::mutex m_mutex;
};
#endif
}  // namespace

int main(int argc, char * argv[])
{
  // Our double parsing code (base/string_utils.hpp) needs dots as a floating point delimiters, not commas.
  // TODO: Refactor our doubles parsing code to use locale-independent delimiters.
  // For example, https://github.com/google/double-conversion can be used.
  // See http://dbaron.org/log/20121222-locale for more details.
  std::setlocale(LC_NUMERIC, "C");

  Platform & platform = GetPlatform();

  LOG(LINFO, ("Organic Maps: Developer Sandbox", platform.Version(), "detected CPU cores:", platform.CpuCores()));

  gflags::SetUsageMessage("Developer Sandbox.");
  gflags::SetVersionString(platform.Version());
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  if (!FLAGS_lang.empty())
    languages::SetPreferredLanguageOverride(FLAGS_lang);

  if (!FLAGS_resources_path.empty())
    platform.SetResourceDir(FLAGS_resources_path);
  if (!FLAGS_data_path.empty())
    platform.SetWritableDirForTests(FLAGS_data_path);

  if (auto const logLevel = base::FromString(FLAGS_log_abort_level); logLevel)
    base::g_LogAbortLevel = *logLevel;
  else
    LOG(LCRITICAL, ("Invalid log level:", FLAGS_log_abort_level));

#if defined(OMIM_OS_LINUX)
  auto guiThread = std::make_unique<LinuxGuiThread>();
  auto guiThreadPtr = guiThread.get();
  platform.SetGuiThread(std::move(guiThread));
  if (FLAGS_smoke_test)
  {
    uint32_t calls = 0;
    base::TaskLoop::Task const task = [&]()
    {
      ++calls;
      guiThreadPtr->Push([&]() { ++calls; });
    };
    guiThreadPtr->Push(task);
    guiThreadPtr->ExecuteTasks();
    CHECK_EQUAL(calls, 1, ("Nested GUI tasks must run in the next batch"));
    guiThreadPtr->ExecuteTasks();
    CHECK_EQUAL(calls, 2, ("Nested GUI task was lost"));
  }
#endif

  // Init GLFW.
  glfwSetErrorCallback(errorCallback);
#if defined(OMIM_OS_LINUX)
  // Give GLFW the same loader used by Drape before either creates a Vulkan instance.
  CHECK(InitVulkan(), ("Could not initialize Vulkan loader"));
  glfwInitVulkanLoader(vkGetInstanceProcAddr);
#endif
  if (!glfwInit())
    return -1;
#if defined(OMIM_OS_LINUX)
  CHECK_EQUAL(glfwGetPlatform(), GLFW_PLATFORM_WAYLAND, ("The Linux sandbox requires Wayland"));
#endif
#if defined(OMIM_OS_MAC)
  auto const initialApi = dp::ApiVersion::Metal;
#else
  auto const initialApi = dp::ApiVersion::Vulkan;
#endif
  // Window dimensions are logical coordinates, independent of monitor pixel resolution.
  GlfwWindow windowOwner(1280, 720);
  windowOwner.Create(initialApi);
  GLFWwindow * window = windowOwner.GetWindows().m_visible;
  int windowWidth = 0, windowHeight = 0;
  glfwGetWindowSize(window, &windowWidth, &windowHeight);
  int fbWidth = 0, fbHeight = 0;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
  float xs = 1.0f, ys = 1.0f;
  glfwGetWindowContentScale(window, &xs, &ys);
  float visualScale = std::max(xs, ys);
#if !defined(OMIM_OS_LINUX)
  auto * monitor = glfwGetPrimaryMonitor();
  CHECK(monitor, ("No primary monitor"));
  glfwSetGamma(monitor, 1.0f);
#endif

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsClassic();

  platform.SetupMeasurementSystem();

  bool outvalue;
  if (!settings::Get(settings::kDeveloperMode, outvalue))
    settings::Set(settings::kDeveloperMode, true);

  FrameworkParams frameworkParams;
  Framework framework(frameworkParams);

  ImguiRenderer imguiRenderer;
#if defined(OMIM_OS_LINUX)
  std::atomic<uint32_t> smokeFrames = 0;
#endif
  Framework::DrapeCreationParams drapeParams{
      .m_apiVersion = initialApi,
      .m_visualScale = visualScale,
      .m_surfaceWidth = fbWidth,
      .m_surfaceHeight = fbHeight,
      .m_widgetsInitInfo = {},
      .m_hints = {},
      .m_renderInjectionHandler = [&](ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> textureManager,
                                      ref_ptr<gpu::ProgramManager> programManager, bool shutdown)
  {
    if (shutdown)
      imguiRenderer.Reset();
    else
    {
      imguiRenderer.Render(context, textureManager, programManager);
#if defined(OMIM_OS_LINUX)
      if (FLAGS_smoke_test)
        ++smokeFrames;
#endif
    }
  }};
  auto UpdateGuiSkin = [&]()
  {
    gui::Skin guiSkin(gui::ResolveGuiSkinFile("default"), visualScale);
    guiSkin.Resize(fbWidth, fbHeight);
    drapeParams.m_widgetsInitInfo.clear();
    guiSkin.ForEach([&](gui::EWidget widget, gui::Position const & pos)
    { drapeParams.m_widgetsInitInfo[widget] = pos; });
    drapeParams.m_widgetsInitInfo[gui::WIDGET_SCALE_FPS_LABEL] = gui::Position(dp::LeftTop);
  };

  drape_ptr<dp::GraphicsContextFactory> contextFactory;
  auto CreateDrapeEngine = [&](dp::ApiVersion version)
  {
    drapeParams.m_apiVersion = version;
    drapeParams.m_visualScale = visualScale;
    drapeParams.m_surfaceWidth = fbWidth;
    drapeParams.m_surfaceHeight = fbHeight;
    UpdateGuiSkin();
    contextFactory = CreateContextFactory(windowOwner.GetWindows(), drapeParams.m_apiVersion,
                                          m2::PointU(static_cast<uint32_t>(drapeParams.m_surfaceWidth),
                                                     static_cast<uint32_t>(drapeParams.m_surfaceHeight)));
#if defined(OMIM_OS_LINUX)
    contextFactory->SetPresentAvailable(false);
#endif
    auto params = drapeParams;
    framework.CreateDrapeEngine(make_ref(contextFactory), std::move(params));
    OnCreateDrapeEngine(window, version, make_ref(contextFactory));
    framework.SetRenderingEnabled(nullptr);
    framework.MakeFrameActive();
  };

  auto DestroyDrapeEngine = [&]()
  {
    // Context destruction runs on enabled workers, including after a temporary pause.
    framework.SetRenderingEnabled();
    framework.SetRenderingDisabled(true);
    framework.DestroyDrapeEngine();
    PrepareDestroyContextFactory(make_ref(contextFactory));
    contextFactory.reset();
  };

  auto UpdatePresentAvailability = [&]()
  {
    bool const available = fbWidth > 0 && fbHeight > 0 && !glfwGetWindowAttrib(window, GLFW_ICONIFIED);
    contextFactory->SetPresentAvailable(available);
    framework.MakeFrameActive();
  };
  handlers.onIconify = UpdatePresentAvailability;

  bool resizePending = false;
  bool scalePending = false;
  // Callbacks only record metrics; resource updates run after native configuration completes.
  handlers.onResize = [&](int w, int h)
  {
    fbWidth = w;
    fbHeight = h;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    resizePending = true;
  };

  handlers.onContentScale = [&](float xscale, float yscale)
  {
    xs = xscale;
    ys = yscale;
    visualScale = std::max(xs, ys);
    scalePending = true;
  };

  auto ApplyWindowChanges = [&]()
  {
    if (scalePending)
    {
      scalePending = false;
#if defined(OMIM_OS_MAC)
      UpdateContentScale(window, xs);
#endif
#if defined(OMIM_OS_LINUX)
      // Visual-scale changes recreate context-dependent resources on the workers.
      // Presentation stays unavailable until window configuration and resizing finish.
      framework.SetRenderingEnabled();
#endif
      framework.UpdateVisualScale(visualScale);
#if defined(OMIM_OS_LINUX)
      framework.SetRenderingDisabled(false);
#endif
      int w, h;
      glfwGetFramebufferSize(window, &w, &h);
      handlers.onResize(w, h);
    }

    if (!resizePending)
      return;
    resizePending = false;
#if !defined(OMIM_OS_LINUX)
    UpdatePresentAvailability();
#endif
    if (fbWidth > 0 && fbHeight > 0)
    {
      UpdateSize(make_ref(contextFactory), fbWidth, fbHeight);
      framework.OnSize(fbWidth, fbHeight);

      UpdateGuiSkin();
      gui::TWidgetsLayoutInfo layout;
      for (auto const & [widget, pos] : drapeParams.m_widgetsInitInfo)
        layout[widget] = pos.m_pixelPivot;
      framework.SetWidgetLayout(std::move(layout));
      framework.MakeFrameActive();
    }
  };

  auto ToFramebuffer = [&](double & x, double & y)
  {
    if (windowWidth <= 0 || windowHeight <= 0 || fbWidth <= 0 || fbHeight <= 0)
      return false;
    x *= static_cast<double>(fbWidth) / windowWidth;
    y *= static_cast<double>(fbHeight) / windowHeight;
    return true;
  };

  // Location handler
  std::optional<ms::LatLon> lastLatLon;
  bool bearingEnabled = false;
  float bearing = 0.0f;
  auto setUserLocation = [&]()
  {
    if (lastLatLon)
    {
      framework.OnLocationUpdate(location::GpsInfo{.m_source = location::EUser,
                                                   .m_timestamp = base::Timer::LocalTime(),
                                                   .m_latitude = lastLatLon->m_lat,
                                                   .m_longitude = lastLatLon->m_lon,
                                                   .m_horizontalAccuracy = 10,
                                                   .m_bearing = bearingEnabled ? bearing : -1.0f});
      if (bearingEnabled)
        framework.OnCompassUpdate(location::CompassInfo{.m_bearing = math::DegToRad(bearing)});
    }
  };

  // Download maps handler
  std::string downloadButtonLabel;
  std::string retryButtonLabel;
  std::string downloadStatusLabel;
  storage::CountryId lastCountry;
  auto const onCountryChanged = [&](storage::CountryId const & countryId)
  {
    downloadButtonLabel.clear();
    retryButtonLabel.clear();
    downloadStatusLabel.clear();

    lastCountry = countryId;
    if (!storage::IsCountryIdValid(countryId))
      return;

    auto const & storage = framework.GetStorage();
    auto status = storage.CountryStatusEx(countryId);
    auto const & countryName = countryId;

    if (status == storage::Status::NotDownloaded)
    {
      std::string units;
      size_t sizeToDownload = 0;
      FormatMapSize(storage.CountrySizeInBytes(countryId).second, units, sizeToDownload);
      std::stringstream str;
      str << "Download (" << countryName << ") " << sizeToDownload << units;
      downloadButtonLabel = str.str();
    }
    else if (status == storage::Status::InQueue)
    {
      std::stringstream str;
      str << countryName << " is waiting for downloading";
      downloadStatusLabel = str.str();
    }
    else if (status != storage::Status::Downloading && status != storage::Status::OnDisk &&
             status != storage::Status::OnDiskOutOfDate)
    {
      std::stringstream str;
      str << "Retry to download " << countryName;
      retryButtonLabel = str.str();
    }
  };
  framework.SetCurrentCountryChangedListener(onCountryChanged);

  framework.GetStorage().Subscribe(
      [&](storage::CountryId const & countryId)
  {
    // Storage also calls notifications for parents, but we are interested in leafs only.
    if (framework.GetStorage().IsLeaf(countryId))
      onCountryChanged(countryId);
  }, [&](storage::CountryId const & countryId, downloader::Progress const & progress)
  {
    std::stringstream str;
    str << "Downloading (" << countryId << ") " << (progress.m_bytesDownloaded * 100 / progress.m_bytesTotal) << "%";
    downloadStatusLabel = str.str();
    framework.MakeFrameActive();
  });

  // Handle mouse buttons.
  bool touchActive = false;
  int touchMods = 0;
  bool setUpLocationByLeftClick = false;
  handlers.onMouseButton = [&](double x, double y, int button, int action, int mods)
  {
    if (ImGui::GetIO().WantCaptureMouse)
    {
      framework.MakeFrameActive();
      return;
    }

    if (!contextFactory || !ToFramebuffer(x, y))
      return;
    lastLatLon = mercator::ToLatLon(framework.PtoG(m2::PointD(x, y)));

    if (setUpLocationByLeftClick)
    {
      setUserLocation();
      return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
      framework.TouchEvent(GetTouchEvent(framework, x, y, mods, df::TouchEvent::TOUCH_DOWN));
      touchActive = true;
      touchMods = mods;
    }

    if (touchActive && action == GLFW_RELEASE)
    {
      framework.TouchEvent(GetTouchEvent(framework, x, y, 0, df::TouchEvent::TOUCH_UP));
      touchActive = false;
      touchMods = 0;
    }
  };

  // Handle mouse moving.
  handlers.onMouseMove = [&](double x, double y)
  {
    if (ImGui::GetIO().WantCaptureMouse)
      framework.MakeFrameActive();

    if (!contextFactory || !ToFramebuffer(x, y))
      return;
    if (touchActive)
      framework.TouchEvent(GetTouchEvent(framework, x, y, touchMods, df::TouchEvent::TOUCH_MOVE));
  };

  // Handle scroll.
  handlers.onScroll = [&](double x, double y, double xOffset, double yOffset)
  {
    if (ImGui::GetIO().WantCaptureMouse)
    {
      framework.MakeFrameActive();
      return;
    }

    if (!contextFactory || !ToFramebuffer(x, y))
      return;
    constexpr double kSensitivity = 0.01;
    double const factor = yOffset * kSensitivity;
    framework.Scale(exp(factor), m2::PointD(x, y), false);
  };

  // Keys.
  handlers.onKeyboardButton = [&](int key, int scancode, int action, int mods) {};

  // imGui UI
  static bool enableDebugRectRendering = false;
  static bool enableAA = false;
  static int currentTileBackground = 0;
  int currentAPI = 0;
  std::optional<dp::ApiVersion> pendingApi;
  auto imGuiUI = [&]()
  {
    ImGui::SetNextWindowPos(ImVec2(5, 20), ImGuiCond_Appearing);
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // Drape controls
    char const * apiLabels[] = {
#if defined(OMIM_OS_MAC)
        "Metal", "Vulkan", "OpenGL"
#elif defined(OMIM_OS_LINUX)
        "Vulkan", "OpenGL"
#elif defined(OMIM_OS_WINDOWS)
        "Vulkan"
#endif
    };
    if (ImGui::Combo("API", &currentAPI, apiLabels, IM_ARRAYSIZE(apiLabels)))
    {
      auto const apiVersion = GetApiVersion(apiLabels[currentAPI]);
      if (drapeParams.m_apiVersion != apiVersion)
        pendingApi = apiVersion;
    }
    if (ImGui::Checkbox("Debug rect rendering", &enableDebugRectRendering))
      framework.EnableDebugRectRendering(enableDebugRectRendering);
    if (ImGui::Checkbox("Antialiasing", &enableAA))
      framework.GetDrapeEngine()->SetPosteffectEnabled(df::PostprocessRenderer::Antialiasing, enableAA);
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();

    // Map controls
    if (ImGui::Button("Scale +"))
      framework.Scale(Framework::SCALE_MAG, true);
    ImGui::SameLine();
    if (ImGui::Button("Scale -"))
      framework.Scale(Framework::SCALE_MIN, true);
    ImGui::Checkbox("Set up location by left click", &setUpLocationByLeftClick);
    if (setUpLocationByLeftClick)
    {
      if (ImGui::Checkbox("Bearing", &bearingEnabled))
        setUserLocation();
      ImGui::SameLine();
      if (ImGui::SliderFloat(" ", &bearing, 0.0f, 360.0f, "%.1f"))
        setUserLocation();
    }
    ImGui::Text("My positon mode: %s", GetMyPoisitionText(framework.GetMyPositionMode()).data());
    if (ImGui::Button("Next Position Mode"))
      framework.SwitchMyPositionNextMode();
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();

    // No downloading on Linux at the moment, need to implement http_thread without Qt.
#if !defined(OMIM_OS_LINUX)
    // Download controls
    if (!downloadButtonLabel.empty())
    {
      if (ImGui::Button(downloadButtonLabel.c_str()))
        framework.GetStorage().DownloadNode(lastCountry);
    }
    if (!retryButtonLabel.empty())
    {
      if (ImGui::Button(retryButtonLabel.c_str()))
        framework.GetStorage().RetryDownloadNode(lastCountry);
    }
    if (!downloadStatusLabel.empty())
      ImGui::Text("%s", downloadStatusLabel.c_str());
    if (!downloadButtonLabel.empty() || !retryButtonLabel.empty() || !downloadStatusLabel.empty())
    {
      ImGui::NewLine();
      ImGui::Separator();
      ImGui::NewLine();
    }
#endif

    char const * tileBackgroundLabels[] = {"Default", "Satellite"};
    if (ImGui::Combo("Tile Background", &currentTileBackground, tileBackgroundLabels,
                     IM_ARRAYSIZE(tileBackgroundLabels)))
    {
      framework.GetDrapeEngine()->SetTileBackgroundMode(static_cast<dp::BackgroundMode>(currentTileBackground),
                                                        0.5f /* satelliteAreaOpacity */);
    }
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();

    ImGui::End();
  };

  auto AttachWindow = [&](dp::ApiVersion api)
  {
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *, int w, int h) { handlers.onResize(w, h); });
    glfwSetWindowIconifyCallback(window, [](GLFWwindow *, int) { handlers.onIconify(); });
    glfwSetWindowContentScaleCallback(
        window, [](GLFWwindow *, float xscale, float yscale) { handlers.onContentScale(xscale, yscale); });
    glfwSetMouseButtonCallback(window, [](GLFWwindow * wnd, int button, int action, int mods)
    {
      double x, y;
      glfwGetCursorPos(wnd, &x, &y);
      handlers.onMouseButton(x, y, button, action, mods);
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow *, double x, double y) { handlers.onMouseMove(x, y); });
    glfwSetScrollCallback(window, [](GLFWwindow * wnd, double xoffset, double yoffset)
    {
      double x, y;
      glfwGetCursorPos(wnd, &x, &y);
      handlers.onScroll(x, y, xoffset, yoffset);
    });
    glfwSetKeyCallback(window, [](GLFWwindow *, int key, int scancode, int action, int mods)
    { handlers.onKeyboardButton(key, scancode, action, mods); });
    bool initialized;
    if (glfwGetWindowAttrib(window, GLFW_CLIENT_API) == GLFW_OPENGL_API)
      initialized = ImGui_ImplGlfw_InitForOpenGL(window, true);
    else if (api == dp::ApiVersion::Vulkan)
      initialized = ImGui_ImplGlfw_InitForVulkan(window, true);
    else
      initialized = ImGui_ImplGlfw_InitForOther(window, true);
    CHECK(initialized, ("Failed to initialize ImGui GLFW backend"));
  };
  AttachWindow(initialApi);
  CreateDrapeEngine(initialApi);

#if defined(OMIM_OS_LINUX)
  uint32_t smokeStep = 0;
  uint32_t smokeLastFrame = 0;
  auto smokeNextStep = std::chrono::steady_clock::now() + std::chrono::seconds(1);
  auto const smokeTimeout = smokeNextStep + std::chrono::seconds(60);
#endif

  // Main loop.
  while (!glfwWindowShouldClose(window))
  {
#if defined(OMIM_OS_LINUX)
    // Wayland configure acknowledgements and surface commits must not overlap GPU frames.
    framework.SetRenderingDisabled(false);
    contextFactory->SetPresentAvailable(false);
#endif
    windowOwner.PollEvents();
#if defined(OMIM_OS_LINUX)
    guiThreadPtr->ExecuteTasks();
#endif
    ApplyWindowChanges();

    // Render imGui UI
    ImGui_ImplGlfw_NewFrame();
    ImGuiIO & io = ImGui::GetIO();
    io.IniFilename = nullptr;
    imguiRenderer.Update(imGuiUI);

#if defined(OMIM_OS_LINUX)
    if (FLAGS_smoke_test)
    {
      auto const now = std::chrono::steady_clock::now();
      CHECK(now < smokeTimeout, ("Sandbox graphics smoke test timed out", smokeStep));
      // Recovery and minimized-window teardown must not depend on renderer progress.
      if (now >= smokeNextStep && (smokeStep == 5 || smokeStep == 13 || smokeStep == 14 || smokeStep == 21 ||
                                   smokeStep == 22 || smokeFrames >= smokeLastFrame + 10))
      {
        LOG(LINFO, ("Sandbox smoke step", smokeStep, "API", drapeParams.m_apiVersion, "framebuffer", fbWidth, fbHeight,
                    "ImGui scale", io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y));
        switch (smokeStep)
        {
        case 0:
          enableDebugRectRendering = true;
          enableAA = true;
          framework.EnableDebugRectRendering(enableDebugRectRendering);
          framework.GetDrapeEngine()->SetPosteffectEnabled(df::PostprocessRenderer::Antialiasing, enableAA);
          glfwRestoreWindow(window);
          break;
        case 1: glfwSetWindowSize(window, 900, 600); break;
        case 2: glfwMaximizeWindow(window); break;
        case 3: glfwRestoreWindow(window); break;
        case 4: handlers.onResize(0, 0); break;
        case 5:
        {
          CHECK_EQUAL(smokeFrames.load(), smokeLastFrame, ("Rendering continued with a zero-size framebuffer"));
          int width, height;
          glfwGetFramebufferSize(window, &width, &height);
          handlers.onResize(width, height);
          break;
        }
        case 6: glfwSetWindowSize(window, 800, 600); break;
        case 7:
          CHECK_EQUAL(windowWidth, 800, ());
          CHECK_EQUAL(windowHeight, 600, ());
          glfwMaximizeWindow(window);
          break;
        case 8:
          CHECK_EQUAL(glfwGetWindowAttrib(window, GLFW_MAXIMIZED), GLFW_TRUE, ());
          pendingApi = dp::ApiVersion::OpenGLES3;
          currentAPI = 1;
          break;
        case 9:
          CHECK_EQUAL(glfwGetWindowAttrib(window, GLFW_MAXIMIZED), GLFW_TRUE, ());
          glfwRestoreWindow(window);
          break;
        case 10:
          CHECK_EQUAL(windowWidth, 800, ("Restored width after API switch"));
          CHECK_EQUAL(windowHeight, 600, ("Restored height after API switch"));
          pendingApi = dp::ApiVersion::Vulkan;
          currentAPI = 0;
          break;
        case 11:
          pendingApi = dp::ApiVersion::OpenGLES3;
          currentAPI = 1;
          break;
        case 12: glfwIconifyWindow(window); break;
        case 13: handlers.onResize(0, 0); break;
        case 14:
          pendingApi = dp::ApiVersion::Vulkan;
          currentAPI = 0;
          break;
        case 15: handlers.onContentScale(visualScale * 1.5f, visualScale * 1.5f); break;
        case 16:
        case 19:
        {
          float xscale, yscale;
          glfwGetWindowContentScale(window, &xscale, &yscale);
          handlers.onContentScale(xscale, yscale);
          break;
        }
        case 17:
          pendingApi = dp::ApiVersion::OpenGLES3;
          currentAPI = 1;
          break;
        case 18: handlers.onContentScale(visualScale * 1.5f, visualScale * 1.5f); break;
        case 20:
          glfwIconifyWindow(window);
          handlers.onResize(0, 0);
          break;
        case 21:
          pendingApi = dp::ApiVersion::Vulkan;
          currentAPI = 0;
          break;
        default:
          glfwIconifyWindow(window);
          handlers.onResize(0, 0);
          glfwSetWindowShouldClose(window, GLFW_TRUE);
          break;
        }
        smokeLastFrame = smokeFrames;
        ++smokeStep;
        smokeNextStep = now + std::chrono::seconds(1);
      }
    }
#endif

    ApplyWindowChanges();
    // Recreate graphics only after the ImGui frame and GLFW callbacks have finished.
    if (pendingApi)
    {
      framework.SaveViewport();
      touchActive = false;
      touchMods = 0;
      DestroyDrapeEngine();
      ImGui_ImplGlfw_Shutdown();
      windowOwner.Destroy();
      windowOwner.Create(*pendingApi);
      window = windowOwner.GetWindows().m_visible;
      glfwGetWindowSize(window, &windowWidth, &windowHeight);
      glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
      glfwGetWindowContentScale(window, &xs, &ys);
      visualScale = std::max(xs, ys);
      resizePending = false;
      scalePending = false;
      AttachWindow(*pendingApi);
      CreateDrapeEngine(*pendingApi);
      framework.EnableDebugRectRendering(enableDebugRectRendering);
      framework.GetDrapeEngine()->SetPosteffectEnabled(df::PostprocessRenderer::Antialiasing, enableAA);
      framework.GetDrapeEngine()->SetTileBackgroundMode(static_cast<dp::BackgroundMode>(currentTileBackground),
                                                        0.5f /* satelliteAreaOpacity */);
      pendingApi.reset();
    }
#if defined(OMIM_OS_LINUX)
    bool const canPresent = fbWidth > 0 && fbHeight > 0;
    contextFactory->SetPresentAvailable(canPresent);
    if (canPresent)
      framework.SetRenderingEnabled();
    else
      framework.SetRenderingDisabled(false);
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 30));
  }

#if defined(OMIM_OS_LINUX)
  CHECK(!FLAGS_smoke_test || smokeStep == 23, ("Sandbox closed before completing the graphics smoke test", smokeStep));
#endif
  framework.EnterBackground();
  DestroyDrapeEngine();

  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  windowOwner.Destroy();
#if defined(OMIM_OS_LINUX)
  if (FLAGS_smoke_test)
    LOG(LINFO, ("Sandbox graphics smoke test passed"));
#endif
  glfwTerminate();
  return 0;
}
