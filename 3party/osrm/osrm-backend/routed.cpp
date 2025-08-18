/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "library/osrm.hpp"
#include "server/server.hpp"
#include "util/git_sha.hpp"
#include "util/routed_options.hpp"
#include "util/simple_logger.hpp"

#ifdef __linux__
#include <sys/mman.h>
#endif

#include <cstdlib>

#include <signal.h>

#include <chrono>
#include <future>
#include <iostream>
#include <thread>

#ifdef _WIN32
boost::function0<void> console_ctrl_function;

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
{
    switch (ctrl_type)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        console_ctrl_function();
        return TRUE;
    default:
        return FALSE;
    }
}
#endif

int main(int argc, const char *argv[])
{
    try
    {
        LogPolicy::GetInstance().Unmute();

        bool trial_run = false;
        std::string ip_address;
        int ip_port, requested_thread_num;

        libosrm_config lib_config;

        const unsigned init_result = GenerateServerProgramOptions(
            argc, argv, lib_config.server_paths, ip_address, ip_port, requested_thread_num,
            lib_config.use_shared_memory, trial_run, lib_config.max_locations_distance_table,
            lib_config.max_locations_map_matching);
        if (init_result == INIT_OK_DO_NOT_START_ENGINE)
        {
            return 0;
        }
        if (init_result == INIT_FAILED)
        {
            return 1;
        }

#ifdef __linux__
        const int lock_flags = MCL_CURRENT | MCL_FUTURE;
        if (-1 == mlockall(lock_flags))
        {
            SimpleLogger().Write(logWARNING) << argv[0] << " could not be locked to RAM";
        }
#endif
        SimpleLogger().Write() << "starting up engines, " << g_GIT_DESCRIPTION;

        if (lib_config.use_shared_memory)
        {
            SimpleLogger().Write(logDEBUG) << "Loading from shared memory";
        }

        SimpleLogger().Write(logDEBUG) << "Threads:\t" << requested_thread_num;
        SimpleLogger().Write(logDEBUG) << "IP address:\t" << ip_address;
        SimpleLogger().Write(logDEBUG) << "IP port:\t" << ip_port;
#ifndef _WIN32
        int sig = 0;
        sigset_t new_mask;
        sigset_t old_mask;
        sigfillset(&new_mask);
        pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
#endif

        OSRM osrm_lib(lib_config);
        auto routing_server = Server::CreateServer(ip_address, ip_port, requested_thread_num);

        routing_server->GetRequestHandlerPtr().RegisterRoutingMachine(&osrm_lib);

        if (trial_run)
        {
            SimpleLogger().Write() << "trial run, quitting after successful initialization";
        }
        else
        {
            std::packaged_task<int()> server_task([&]() -> int
                                                  {
                                                      routing_server->Run();
                                                      return 0;
                                                  });
            auto future = server_task.get_future();
            std::thread server_thread(std::move(server_task));

#ifndef _WIN32
            sigset_t wait_mask;
            pthread_sigmask(SIG_SETMASK, &old_mask, 0);
            sigemptyset(&wait_mask);
            sigaddset(&wait_mask, SIGINT);
            sigaddset(&wait_mask, SIGQUIT);
            sigaddset(&wait_mask, SIGTERM);
            pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
            SimpleLogger().Write() << "running and waiting for requests";
            sigwait(&wait_mask, &sig);
#else
            // Set console control handler to allow server to be stopped.
            console_ctrl_function = std::bind(&Server::Stop, routing_server);
            SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
            SimpleLogger().Write() << "running and waiting for requests";
            routing_server->Run();
#endif
            SimpleLogger().Write() << "initiating shutdown";
            routing_server->Stop();
            SimpleLogger().Write() << "stopping threads";

            auto status = future.wait_for(std::chrono::seconds(2));

            if (status == std::future_status::ready)
            {
                server_thread.join();
            }
            else
            {
                SimpleLogger().Write(logWARNING) << "Didn't exit within 2 seconds. Hard abort!";
                server_task.reset(); // just kill it
            }
        }

        SimpleLogger().Write() << "freeing objects";
        routing_server.reset();
        SimpleLogger().Write() << "shutdown completed";
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "exception: " << e.what();
        return 1;
    }
#ifdef __linux__
    munlockall();
#endif

    return 0;
}
