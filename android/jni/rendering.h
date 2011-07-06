#pragma once

#include "../../map/drawer_yg.hpp"

#include "../../yg/rendercontext.hpp"
#include "../../yg/resource_manager.hpp"
#include "../../std/shared_ptr.hpp"


shared_ptr<yg::ResourceManager> CreateResourceManager();

shared_ptr<yg::gl::RenderContext> CreateRenderContext();

shared_ptr<DrawerYG> CreateDrawer(shared_ptr<yg::ResourceManager> pRM);
