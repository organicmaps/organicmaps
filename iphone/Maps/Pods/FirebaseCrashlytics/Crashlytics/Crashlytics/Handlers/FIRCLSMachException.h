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

#include "FIRCLSFeatures.h"

#pragma once

#if CLS_MACH_EXCEPTION_SUPPORTED

#include <mach/mach.h>
#include <pthread.h>
#include <stdbool.h>

// must be at least PTHREAD_STACK_MIN size
#define CLS_MACH_EXCEPTION_HANDLER_STACK_SIZE (256 * 1024)

#pragma mark Structures
#pragma pack(push, 4)
typedef struct {
  mach_msg_header_t head;
  /* start of the kernel processed data */
  mach_msg_body_t msgh_body;
  mach_msg_port_descriptor_t thread;
  mach_msg_port_descriptor_t task;
  /* end of the kernel processed data */
  NDR_record_t NDR;
  exception_type_t exception;
  mach_msg_type_number_t codeCnt;
  mach_exception_data_type_t code[EXCEPTION_CODE_MAX];
  mach_msg_trailer_t trailer;
} MachExceptionMessage;

typedef struct {
  mach_msg_header_t head;
  NDR_record_t NDR;
  kern_return_t retCode;
} MachExceptionReply;
#pragma pack(pop)

typedef struct {
  mach_msg_type_number_t count;
  exception_mask_t masks[EXC_TYPES_COUNT];
  exception_handler_t ports[EXC_TYPES_COUNT];
  exception_behavior_t behaviors[EXC_TYPES_COUNT];
  thread_state_flavor_t flavors[EXC_TYPES_COUNT];
} FIRCLSMachExceptionOriginalPorts;

typedef struct {
  mach_port_t port;
  pthread_t thread;
  const char* path;

  exception_mask_t mask;
  FIRCLSMachExceptionOriginalPorts originalPorts;
} FIRCLSMachExceptionReadContext;

#pragma mark - API
void FIRCLSMachExceptionInit(FIRCLSMachExceptionReadContext* context, exception_mask_t ignoreMask);
exception_mask_t FIRCLSMachExceptionMaskForSignal(int signal);

void FIRCLSMachExceptionCheckHandlers(void);

#else

#define CLS_MACH_EXCEPTION_HANDLER_STACK_SIZE 0

#endif
