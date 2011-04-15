/*
 *  global.hpp
 *  sloynik
 *
 *  Created by Yury Melnichek on 20.01.11.
 *  Copyright 2011 -. All rights reserved.
 *
 */

#pragma once

namespace sl
{
  class SloynikEngine;
}

sl::SloynikEngine * CreateSloynikEngine();
sl::SloynikEngine * GetSloynikEngine();
void SetSloynikEngine(sl::SloynikEngine *);
