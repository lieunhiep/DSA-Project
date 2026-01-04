#include <iostream>
#include <vector>
#include <fstream>
#include <queue>
#include <algorithm>
#include <cstdio>

#ifdef _WIN32
    #include <windows.h>
#elif __APPLE__
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <mach/mach.h>
#endif

using namespace std;

/* ================= TÍNH RAM KHẢ DỤNG ================= */
long long getAvailableRAM() {
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return (long long)status.ullAvailPhys;
#elif __APPLE__
    vm_statistics64_data_t vm_stats;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    int page_size;
    size_t len = sizeof(page_size);
    sysctlbyname("hw.pagesize", &page_size, &len, NULL, 0);

    if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
        (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
        return (long long)(vm_stats.free_count + vm_stats.inactive_count) * page_size;
    }
    return 1024LL * 1024 * 1024;
#endif
}

/* ================= HEAP SORT ================= */
void heapify(vector<int>& arr, int n, int i) {
    int largest = i;
    int l = 2 * i + 1;
    int r = 2 * i + 2;

    if (l < n && arr[l] > arr[largest]) largest = l;
    if (r < n && arr[r] > arr[largest]) largest = r;

    if (largest != i) {
        swap(arr[i], arr[largest]);
        heapify(arr, n, largest);
    }
}

void heapSort(vector<int>& arr) {
    int n = arr.size();

    for (int i = n / 2 - 1; i >= 0; i--)
        heapify(arr, n, i);

    for (int i = n - 1; i > 0; i--) {
        swap(arr[0], arr[i]);
        heapify(arr, i, 0);
    }
}

/* ================= MERGE ================= */
struct Node {
    int value;
    int idx;
    bool operator>(const Node& other) const {
        return value > other.value;
    }
};

string mergeBatch(const vector<string>& files, int pass, int batch) {
    string outName = "merge_p" + to_string(pass) + "_" + to_string(batch) + ".txt";
    priority_queue<Node, vector<Node>, greater<Node>> pq;
    vector<ifstream> streams(files.size());

    for (int i = 0; i < files.size(); i++) {
        streams[i].open(files[i]);
        int x;
        if (streams[i] >> x) {
            pq.push({x, i});
        }
    }

    ofstream out(outName);
    while (!pq.empty()) {
        Node n = pq.top(); pq.pop();
        out << n.value << '\n';

        int next;
        if (streams[n.idx] >> next) {
            pq.push({next, n.idx});
        }
    }

    for (auto& f : files) remove(f.c_str());
    return outName;
}

/* ================= MAIN SORT ================= */
void sortBigFile(const string& inFile, const string& outFile) {
    long long freeRAM = getAvailableRAM();
    long long ramLimit = (long long)(freeRAM * 0.7);

    cout << "Available RAM: " << freeRAM / (1024 * 1024) << " MB\n";
    cout << "Sorting buffer: " << ramLimit / (1024 * 1024) << " MB\n";

    ifstream in(inFile);
    vector<int> buffer;
    vector<string> runs;

    long long used = 0;
    int runId = 0;
    int x;

    while (in >> x) {
        buffer.push_back(x);
        used += sizeof(int);

        if (used >= ramLimit) {
            heapSort(buffer);
            string name = "run_" + to_string(runId++) + ".txt";
            ofstream out(name);
            for (int v : buffer) out << v << '\n';
            runs.push_back(name);
            buffer.clear();
            used = 0;
        }
    }

    if (!buffer.empty()) {
        heapSort(buffer);
        string name = "run_" + to_string(runId++) + ".txt";
        ofstream out(name);
        for (int v : buffer) out << v << '\n';
        runs.push_back(name);
    }

    /* ===== MULTI-PASS MERGE ===== */
    const int MAX_OPEN = 100;
    int pass = 0;

    while (runs.size() > 1) {
        vector<string> next;
        for (int i = 0; i < runs.size(); i += MAX_OPEN) {
            vector<string> batch(
                runs.begin() + i,
                runs.begin() + min(i + MAX_OPEN, (int)runs.size())
            );
            next.push_back(mergeBatch(batch, pass, i / MAX_OPEN));
        }
        runs.swap(next);
        pass++;
    }

    rename(runs[0].c_str(), outFile.c_str());
    cout << "✔ Sorted to " << outFile << '\n';
}

/* ================= ENTRY ================= */
int main() {
    sortBigFile("bigdata.txt", "sorted_data.txt");
    return 0;
}
