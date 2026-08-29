// C++ backend. React (compiled) runs in the native webview.
// C++ functions are bound into JS via webview bind(); JS awaits them.
//
// Communication pattern:
//   JS  ->  await window.fnName(args)   (args become a JSON array string)
//   C++ ->  returns a JSON string       (becomes the resolved promise value)
#include "webview/webview.h"
#include <string>
#include <chrono>
#include <thread>
#include <cstdint>
#include <cstdio>

#if defined(_WIN32)
  #include <windows.h>
#endif

// ---------------------------------------------------------------------------
// Business logic (the work lives in C++)
// ---------------------------------------------------------------------------
static bool is_prime(uint64_t n) {
    if (n < 2) return false;
    for (uint64_t i = 2; i * i <= n; ++i) if (n % i == 0) return false;
    return true;
}
static uint64_t count_primes(uint64_t n) {
    uint64_t c = 0;
    for (uint64_t i = 2; i <= n; ++i) if (is_prime(i)) ++c;
    return c;
}
static std::string platform_name() {
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#else
    return "Unknown";
#endif
}

// ---------------------------------------------------------------------------
// App
// ---------------------------------------------------------------------------
#if defined(_WIN32)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#else
int main() {
#endif
    webview::webview w(true, nullptr);
    w.set_title("citepp");
    w.set_size(640, 560, WEBVIEW_HINT_NONE);

    // Example 1: JS passes an argument, C++ computes, returns a result.
    //   JS: await window.countPrimes(200000)
    w.bind("countPrimes", [](const std::string& req) -> std::string {
        unsigned long long limit = 0;
        std::sscanf(req.c_str(), "[%llu]", &limit);   // req is e.g. "[200000]"
        auto t0 = std::chrono::steady_clock::now();
        uint64_t primes = count_primes(limit);
        double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t0).count();
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "{\"limit\":%llu,\"primes\":%llu,\"ms\":%.2f}",
            limit, (unsigned long long)primes, ms);
        return buf;
    });

    // Example 2: no arguments, C++ just returns some data.
    //   JS: await window.systemInfo()
    w.bind("systemInfo", [](const std::string&) -> std::string {
        char buf[256];
        std::snprintf(buf, sizeof(buf),
            "{\"platform\":\"%s\",\"cores\":%u}",
            platform_name().c_str(), std::thread::hardware_concurrency());
        return buf;
    });

    w.navigate("file://" + std::string("ui/index.html"));
    w.run();
    return 0;
}
