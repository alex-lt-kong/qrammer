#include "utils.h"

#include <spdlog/spdlog.h>

#include <thread>
#ifdef WIN32
#include <windows.h>
#endif

using namespace std;

void execExternalProgramAsync(const string cmd)
{
    thread th_exec([cmd]() {
        SPDLOG_INFO("Calling external program: [{}] in a separate child process.", cmd);
        int retval = 0;
        try {
#ifdef WIN32
            retval = WinExec(cmd.c_str(), SW_HIDE);
#else
            retval = system(cmd.c_str());
#endif
            SPDLOG_INFO("External program: [{}] returned {}", cmd, retval);
        } catch (const exception &e) {
            SPDLOG_ERROR("Failed calling {}: {}", cmd, e.what());
        }
    });
    th_exec.detach();
}
