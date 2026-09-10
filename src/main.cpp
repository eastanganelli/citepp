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
#elif defined(__APPLE__)
  #include <mach-o/dyld.h>
  #include <climits>
#else
  #include <unistd.h>
  #include <climits>
#endif

// ---------------------------------------------------------------------------
// Locating the UI on disk
//
// The webview can only load the bundle through a file:// URL, and that URL has
// to be absolute: "file://ui/index.html" parses "ui" as the *host*, so the
// platform webview goes looking for a network share and the window stays blank.
// Resolving against the executable's own directory also means the app works no
// matter what the current working directory is (double-clicked, unzipped
// artifact, launched from a shortcut).
// ---------------------------------------------------------------------------
static std::string executable_dir() {
#if defined(_WIN32)
    wchar_t wbuf[MAX_PATH * 4];
    DWORD n = GetModuleFileNameW(nullptr, wbuf, (DWORD)(sizeof(wbuf) / sizeof(wbuf[0])));
    if (n == 0) return ".";
    int len = WideCharToMultiByte(CP_UTF8, 0, wbuf, (int)n, nullptr, 0, nullptr, nullptr);
    std::string path(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wbuf, (int)n, &path[0], len, nullptr, nullptr);
    for (char& c : path) if (c == '\\') c = '/';
#elif defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) return ".";
    std::string path(buf);
#else
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return ".";
    buf[n] = '\0';
    std::string path(buf);
#endif
    auto slash = path.find_last_of('/');
    return slash == std::string::npos ? std::string(".") : path.substr(0, slash);
}

// Percent-encode the characters that are not legal in a URL path, so paths
// containing spaces or non-ASCII characters still resolve.
static std::string to_file_url(const std::string& abs_path) {
    static const char* hex = "0123456789ABCDEF";
    std::string out = "file://";
    if (!abs_path.empty() && abs_path[0] != '/') out += '/';   // file:///C:/...
    for (unsigned char c : abs_path) {
        bool safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '/' || c == ':' ||
                    c == '-' || c == '_' || c == '.' || c == '~';
        if (safe) out += (char)c;
        else { out += '%'; out += hex[c >> 4]; out += hex[c & 0xF]; }
    }
    return out;
}

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

    w.navigate(to_file_url(executable_dir() + "/ui/index.html"));
    w.run();
    return 0;
}
