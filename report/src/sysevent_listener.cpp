/*
 * @Description:
 * @Date: 2022-04-21 19:40:58
 * @LastEditTime: 2022-04-21 19:54:19
 * @FilePath: /wukong/report/src/sysevent_listener.cpp
 * @author: lemon
 */
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

#include "sysevent_listener.h"

#include <nlohmann/json.hpp>

#include "csv_utils.h"
#include "hisysevent.h"

namespace OHOS {
namespace WuKong {
using nlohmann::json;
using OHOS::HiviewDFX::HiSysEvent;

namespace {
template <typename ValueType>
void ValueGet(const json& jsonData, const std::string& key, const json::value_t vt, ValueType& data)
{
    TRACK_LOG_STD();
    if (jsonData.contains(key)) {
        if (vt == jsonData[key].type()) {
            ValueType value = jsonData[key].get<ValueType>();
            data = value;
        }
    }
    TRACK_LOG_END();
}
}  // namespace

void SysEventListener::OnHandle(const std::string& domain, const std::string& eventName, const int eventType,
                                const std::string& eventDetail)
{
    TRACK_LOG_STD();
    TRACK_LOG("----------Exception caught----------");
    WARN_LOG_STR("domain: %s", domain.c_str());
    TRACK_LOG_STR("eventName: %s", eventName.c_str());
    TRACK_LOG_STR("eventType: %s", std::to_string(eventType).c_str());
    TRACK_LOG_STR("eventDetail: %s", eventDetail.c_str());
    TRACK_LOG("------------------------------------");
    CsvUtils::OneLineData data;
    data.domain = domain;
    data.name = eventName;
    switch (eventType) {
        case HiSysEvent::EventType::FAULT:
            data.type = "FAULT";
            break;
        case HiSysEvent::EventType::STATISTIC:
            data.type = "STATISTIC";
            break;
        case HiSysEvent::EventType::SECURITY:
            data.type = "SECURITY";
            break;
        case HiSysEvent::EventType::BEHAVIOR:
            data.type = "BEHAVIOR";
            break;
        default:
            data.type = "UNKNOWN";
    }

    json jsonData = json::parse(eventDetail, nullptr, false);
    if (jsonData == json::value_t::discarded) {
        ERROR_LOG_STR("event detail parse error, the content: %s", eventDetail.c_str());
    } else {
        ValueGet<uint64_t>(jsonData, "time_", json::value_t::number_unsigned, data.time);
        ValueGet<std::string>(jsonData, "tz_", json::value_t::string, data.timeZone);
        ValueGet<uint64_t>(jsonData, "pid_", json::value_t::number_unsigned, data.pid);
        ValueGet<uint64_t>(jsonData, "tid_", json::value_t::number_unsigned, data.tid);
        ValueGet<uint64_t>(jsonData, "uid_", json::value_t::number_unsigned, data.uid);
    }
CsvUtils::WriteOneLine(csvFile, data);
TRACK_LOG_END();
}
void SysEventListener::OnServiceDied()
{
    ERROR_LOG("Listener service Died");
}
}  // namespace WuKong
}  // namespace OHOS
