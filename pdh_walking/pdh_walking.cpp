#include <windows.h>
#include <Pdh.h>
#include <PdhMsg.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

#pragma comment(lib, "pdh.lib")

struct ProcessInfo {
    DWORD pid;
    std::wstring name;
};

class ProcessLister {
public:
    static std::vector<ProcessInfo> GetProcessList() {
        std::vector<ProcessInfo> processList;
        PDH_HQUERY query;
        PDH_HCOUNTER counter;
        PDH_FMT_COUNTERVALUE_ITEM* items = nullptr;
        DWORD itemCount = 0;
        DWORD bufferSize = 0;

        if (PdhOpenQuery(NULL, 0, &query) != ERROR_SUCCESS) {
            return processList;
        }

        if (PdhAddEnglishCounter(query, L"\\Process(*)\\ID Process", 0, &counter) != ERROR_SUCCESS) {
            PdhCloseQuery(query);
            return processList;
        }

        if (PdhCollectQueryData(query) != ERROR_SUCCESS) {
            PdhCloseQuery(query);
            return processList;
        }

        if (PdhGetFormattedCounterArray(counter, PDH_FMT_LONG, &bufferSize, &itemCount, NULL) != PDH_MORE_DATA) {
            PdhCloseQuery(query);
            return processList;
        }

        items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM*>(new char[bufferSize]);

        if (PdhGetFormattedCounterArray(counter, PDH_FMT_LONG, &bufferSize, &itemCount, items) == ERROR_SUCCESS) {
            for (DWORD i = 0; i < itemCount; i++) {
                DWORD pid = static_cast<DWORD>(items[i].FmtValue.longValue);
                if (pid > 0) {
                    std::wstring processName = items[i].szName;
                    size_t pos = processName.find_last_of(L'(');
                    if (pos != std::wstring::npos) {
                        processName = processName.substr(pos + 1);
                        processName.pop_back(); // Remove the trailing ')'
                    }
                    processList.push_back({ pid, processName });
                }
            }
        }

        delete[] reinterpret_cast<char*>(items);
        PdhCloseQuery(query);
        return processList;
    }
};

int main() {
    auto processes = ProcessLister::GetProcessList();

    std::sort(processes.begin(), processes.end(),
        [](const ProcessInfo& a, const ProcessInfo& b) { return a.pid < b.pid; });

    std::wcout << L"PID\tProcess Name" << std::endl;
    std::wcout << L"---\t------------" << std::endl;
    for (const auto& process : processes) {
        std::wcout << process.pid << L"\t" << process.name << std::endl;
    }

    return 0;
}