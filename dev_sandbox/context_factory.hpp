#pragma once

#include "dev_sandbox/glfw_window.hpp"

#include "drape/graphics_context_factory.hpp"
#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"

drape_ptr<dp::GraphicsContextFactory> CreateContextFactory(GlfwWindows const & windows, dp::ApiVersion api,
                                                           m2::PointU size);
void PrepareDestroyContextFactory(ref_ptr<dp::GraphicsContextFactory> contextFactory);
void OnCreateDrapeEngine(GLFWwindow * window, dp::ApiVersion api, ref_ptr<dp::GraphicsContextFactory> contextFactory);
void UpdateContentScale(GLFWwindow * window, float scale);
void UpdateSize(ref_ptr<dp::GraphicsContextFactory> contextFactory, int w, int h);
