/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdio>
#include <sys/stat.h>

#include "string_ex.h"
#include "wukong_define.h"
#include "wukong_shell_command.h"
#include "dfx_signal_handler.h"

using namespace OHOS::WuKong;

static unsigned int NUMBER_TWO = 2;

static void WuKongMutexFile()
{
    int fileExist_ = access("/dev/shm", F_OK);
    if (fileExist_ == 0) {
        DEBUG_LOG("File exist. Now create wukong test mutex.");
    } else {
        const int wukongm_ = mkdir("/dev/shm", 0777);
        DEBUG_LOG("File create. Now create wukong test mutex.");
        if (wukongm_ == -1) {
            DEBUG_LOG("Error creating directory!");
        }
    }
}

static void InitSemaphore(NamedSemaphore& sem, const int count)
{
    bool res = sem.Open();
    int value = 0;
    if (!res) {
        WuKongMutexFile();
        res = sem.Create();
    }
    if (res) {
        DEBUG_LOG("Open Semaphore success");
        value = sem.GetValue();
        if (value > count) {
            DEBUG_LOG_STR("the semaphore value is unvalid (%d), and reopen Semaphore", value);
            res = sem.Create();
        } else {
            DEBUG_LOG_STR("Semaphore Value: (%d)", value);
        }
    }
    sem.Close();
}

static bool IsRunning(NamedSemaphore& sem)
{
    bool result = false;
    sem.Open();
    // the wukong pidof buffer size.
    const int bufferSize = 32;
    int value = sem.GetValue();
    TRACK_LOG_STR("Semaphore Is Open: (%d)", value);
    if (value <= 0) {
        FILE* fp = nullptr;
        fp = popen("pidof wukong", "r");
        TRACK_LOG("Run pidof wukong");
        if (fp == nullptr) {
            ERROR_LOG("popen function failed");
            return true;
        }
        char pid[bufferSize] = {0};
        if (fgets(pid, bufferSize - 1, fp) != nullptr) {
            std::string pidStr(pid);
            pidStr = OHOS::ReplaceStr(pidStr, "\n", " ");
            TRACK_LOG_STR("Wukong Pid: (%s)", pidStr.c_str());
            std::vector<std::string> strs;
            OHOS::SplitStr(pidStr, " ", strs);
            for (auto i : strs) {
                DEBUG_LOG_STR("Pid: (%s)", i.c_str());
            }
            if (strs.size() >= NUMBER_TWO) {
                result = true;
            } else {
                sem.Create();
                result = false;
            }
        } else {
            result = true;
        }
        pclose(fp);
    }
    return result;
}

int main(int argc, char* argv[])
{
    std::shared_ptr<WuKongLogger> WuKonglogger = WuKongLogger::GetInstance();
    // first start logger
    WuKonglogger->SetLevel(LOG_LEVEL_INFO);
    bool isStop = false;
    for (int index = argc - 1; index >= 1; index--) {
        std::string arg = argv[index];
        if (arg == "--track") {
            argv[index][0] = '\0';
            WuKonglogger->SetLevel(LOG_LEVEL_TRACK);
        }
        if (arg == "--debug") {
            argv[index][0] = '\0';
            WuKonglogger->SetLevel(LOG_LEVEL_DEBUG);
        }
        if (arg == "stop") {
            isStop = true;
        }
    }
    if (!WuKonglogger->Start()) {
        return 1;
    }

    NamedSemaphore semRun(SEMPHORE_RUN_NAME, 1);
    InitSemaphore(semRun, 1);
    NamedSemaphore semStop(SEMPHORE_STOP_NAME, 1);
    InitSemaphore(semStop, 1);

    if (isStop) {
        WuKongShellCommand cmd(argc, argv);
        std::cout << cmd.ExecCommand();
    } else {
        if (IsRunning(semRun)) {
            ERROR_LOG("error: wukong has running, allow one program run.");
        } else {
            semRun.Open();
            semRun.Wait();
            WuKongShellCommand cmd(argc, argv);
            std::cout << cmd.ExecCommand();
            semRun.Post();
            semRun.Close();
        }
    }
    WuKonglogger->Stop();
    return 0;
}
