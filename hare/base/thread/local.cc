#include "hare/base/thread/local.h"
#include <hare/base/system_check.h>

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <thread>

#ifdef H_OS_WIN32
#include <Windows.h>
#elif defined(H_OS_LINUX)
#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>
#endif

namespace hare {

thread_local struct ThreadLocal t_data;

namespace current_thread {

    namespace detail {
        static const char c_digits_hex[] = "0123456789ABCDEF";
        static_assert(sizeof(c_digits_hex) == 17, "wrong number of c_digits_hex");

        void convertHex(std::string& buf, uintptr_t value)
        {
            buf.clear();
            auto p = buf.begin();

            do {
                auto lsd = static_cast<int32_t>(value % 16);
                value /= 16;
                *p++ = c_digits_hex[lsd];
            } while (value != 0);

            buf.push_back('x');
            buf.push_back('0');
            std::reverse(buf.begin(), buf.end());
            buf.push_back('\0');
        }
    } // namespace detail

    void cacheThreadId()
    {
        if (t_data.tid_ == 0) {
            std::ostringstream oss;
            oss << std::this_thread::get_id();
            t_data.tid_string_ = oss.str() + "\0";
            t_data.tid_ = std::stoull(t_data.tid_string_);
            detail::convertHex(t_data.tid_string_, t_data.tid_);
        }
    }

    bool isSameThread()
    {
#ifdef H_OS_WIN32
        return tid() == ::GetCurrentProcessId();
#elif defined(H_OS_LINUX)
        return tid() == ::getpid();
#endif
    }

    std::string stackTrace(bool demangle)
    {
        static const int MAX_STACK_FRAMES = 20;
#ifdef H_OS_WIN32
        void* pStack[MAX_STACK_FRAMES];

        auto process = GetCurrentProcess();
        SymInitialize(process, NULL, TRUE);
        auto frames = CaptureStackBackTrace(0, MAX_STACK_FRAMES, pStack, NULL);

        std::ostringstream oss;
        oss << "stack traceback: " << std::endl;
        for (WORD i = 0; i < frames; ++i) {
            auto address = (DWORD64)(pStack[i]);

            DWORD64 displacementSym = 0;
            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            auto pSymbol = (PSYMBOL_INFO)buffer;
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;

            DWORD displacementLine = 0;
            IMAGEHLP_LINE64 line;
            // SymSetOptions(SYMOPT_LOAD_LINES);
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

            if (SymFromAddr(process, address, &displacementSym, pSymbol) && SymGetLineFromAddr64(process, address, &displacementLine, &line)) {
                oss << "\t" << pSymbol->Name << " at " << line.FileName << ":" << line.LineNumber << "(0x" << std::hex << pSymbol->Address << std::dec << ")" << std::endl;
            } else {
                oss << "\terror: " << GetLastError() << std::endl;
            }
        }
        return oss.str();
#elif defined(H_OS_LINUX)
        std::string stack;
        void* frame[MAX_STACK_FRAMES];
        auto nptrs = ::backtrace(frame, MAX_STACK_FRAMES);
        auto strings = ::backtrace_symbols(frame, nptrs);
        if (strings) {
            size_t len = 256;
            auto demangled = demangle ? static_cast<char*>(::malloc(len)) : nullptr;
            for (auto i = 1; i < nptrs; ++i) {
                // skipping the 0-th, which is this function.
                if (demangle) {
                    // https://panthema.net/2008/0901-stacktrace-demangled/
                    // bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
                    char* left_par = nullptr;
                    char* plus = nullptr;
                    for (auto p = strings[i]; *p; ++p) {
                        if (*p == '(')
                            left_par = p;
                        else if (*p == '+')
                            plus = p;
                    }

                    if (left_par && plus) {
                        *plus = '\0';
                        auto status = 0;
                        auto ret = abi::__cxa_demangle(left_par + 1, demangled, &len, &status);
                        *plus = '+';
                        if (status == 0) {
                            demangled = ret; // ret could be realloc()
                            stack.append(strings[i], left_par + 1);
                            stack.append(demangled);
                            stack.append(plus);
                            stack.push_back('\n');
                            continue;
                        }
                    }
                }
                // Fallback to mangled names
                stack.append(strings[i]);
                stack.push_back('\n');
            }
            free(demangled);
            free(strings);
        }
        return stack;
#endif
    }

}
}