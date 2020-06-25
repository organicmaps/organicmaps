// Copyright 2019 Google
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <mach/mach.h>

#include "FIRCLSFile.h"

__BEGIN_DECLS

void FIRCLSHandler(FIRCLSFile* file, thread_t crashedThread, void* uapVoid);
void FIRCLSHandlerAttemptImmediateDelivery(void);

__END_DECLS
