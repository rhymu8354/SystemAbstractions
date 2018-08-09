/**
 * @file ClipboardWin32.cpp
 *
 * This module contains the Win32 specific part of the 
 * implementation of the SystemAbstractions::Clipboard class.
 *
 * © 2016-2018 by Richard Walters
 */

/**
 * Windows.h should always be included first because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h first.
 */
#include <Windows.h>

#include <SystemAbstractions/Clipboard.hpp>

#include <atlbase.h>
#include <tchar.h>

namespace {

    /**
     * This is the name of the window class registered for the special
     * hidden window used to access the system clipboard.
     */
    const LPCTSTR WINDOW_CLASS_NAME = _T("DigitalStirling-Clipboard");

}

namespace SystemAbstractions {

    /**
     * This is the Win32-specific state for the Clipboard class.
     */
    struct Clipboard::Impl {
        // Properties

        /**
         * This is a counter of the number of instances of this class
         * that currently constructed.  It is used to determine when
         * to create and destroy the special hidden window used to
         * access the system clipboard.
         */
        static size_t instances_;

        /**
         * This flag indicates whether or not initialization of the
         * special hidden window used to access the system clipboard
         * succeeded.
         */
        static bool initFailed_;

        /**
         * This is the window class information for the special hidden
         * window used to access the system clipboard.
         */
        static WNDCLASSEX wc_;

        /**
         * This is the handle to the special hidden window used to
         * access the system clipboard.
         */
        static HWND windowHandle_;

        // Lifecycle management

        ~Impl() noexcept {
            if (--instances_ == 0) {
                if (!initFailed_) {
                    if (windowHandle_ != NULL) {
                        DestroyWindow(windowHandle_);
                        windowHandle_ = NULL;
                    }
                    (void)UnregisterClass(WINDOW_CLASS_NAME, GetModuleHandle(NULL));
                }
            }
        }
        Impl(const Impl& other) = delete;
        Impl(Impl&& other) noexcept = delete;
        Impl& operator=(const Impl& other) = default;
        Impl& operator=(Impl&& other) noexcept = default;

        // Methods

        /**
         * This is the constructor.
         */
        Impl() {
            if (instances_++ == 0) {
                ZeroMemory(&wc_, sizeof(wc_));
                wc_.cbSize = sizeof(wc_);
                wc_.lpfnWndProc = WindowProc;
                wc_.hInstance = GetModuleHandle(NULL);
                wc_.lpszClassName = WINDOW_CLASS_NAME;
                const ATOM atom = RegisterClassEx(&wc_);
                initFailed_ = (atom == 0);
                if (!initFailed_) {
                    windowHandle_ = CreateWindowEx(
                        NULL,
                        WINDOW_CLASS_NAME,
                        L"",
                        0,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        0,
                        0,
                        NULL, NULL,
                        GetModuleHandle(NULL),
                        this
                    );
                }
            }
        }

        /**
         * This is the window procedure that handles messages for the
         * special hidden window used to access the system clipboard.
         *
         * @param[in] hWnd
         *     This is the handle to the window.
         *
         * @param[in] message
         *     This identifies the message sent to the window.
         *
         * @param[in] wParam
         *     This is the first parameter of the window message.
         *
         * @param[in] lParam
         *     This is the second parameter of the window message.
         *
         * @return
         *     The result of the window processing the message is returned.
         *     The meaning depends on which message was sent.
         */
        static LRESULT CALLBACK WindowProc(
            HWND hWnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
        ) {
            Clipboard::Impl* self = (Clipboard::Impl*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            switch(message) {
                case WM_CREATE: {
                    CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
                    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)createStruct->lpCreateParams);
                } break;
                case WM_DESTROY: {
                    PostQuitMessage(0);
                } return 0;
                default: break;
            }
            return DefWindowProc(hWnd, message, wParam, lParam);
        }

    };

    size_t Clipboard::Impl::instances_ = 0;
    bool Clipboard::Impl::initFailed_ = false;
    WNDCLASSEX Clipboard::Impl::wc_;
    HWND Clipboard::Impl::windowHandle_ = NULL;

    Clipboard::Clipboard()
        : impl_(new Impl())
    {
    }

    Clipboard::~Clipboard() {
    }

    void Clipboard::Copy(const std::string& s) {
        if (!selectedClipboardOperatingSystemInterface->OpenClipboard(impl_->windowHandle_)) {
            return;
        }
        if (selectedClipboardOperatingSystemInterface->EmptyClipboard()) {
            HGLOBAL dataHandle = GlobalAlloc(GMEM_MOVEABLE, s.length() + 1);
            if (dataHandle != NULL) {
                LPSTR data = (LPSTR)GlobalLock(dataHandle);
                if (data != NULL) {
                    memcpy(data, s.c_str(), s.length() + 1);
                    (void)GlobalUnlock(data);
                    selectedClipboardOperatingSystemInterface->SetClipboardData(CF_TEXT, dataHandle);
                }
            }
        }
        (void)selectedClipboardOperatingSystemInterface->CloseClipboard();
    }

    bool Clipboard::HasString() {
        if (!selectedClipboardOperatingSystemInterface->OpenClipboard(impl_->windowHandle_)) {
            return false;
        }
        const bool hasString = (selectedClipboardOperatingSystemInterface->IsClipboardFormatAvailable(CF_TEXT) != 0);
        (void)selectedClipboardOperatingSystemInterface->CloseClipboard();
        return hasString;
    }

    std::string Clipboard::PasteString() {
        if (!selectedClipboardOperatingSystemInterface->OpenClipboard(impl_->windowHandle_)) {
            return "";
        }
        std::string value;
        if (selectedClipboardOperatingSystemInterface->IsClipboardFormatAvailable(CF_TEXT)) {
            HGLOBAL dataHandle = selectedClipboardOperatingSystemInterface->GetClipboardData(CF_TEXT);
            if (dataHandle != NULL) {
                LPCSTR data = (LPCSTR)GlobalLock(dataHandle);
                if (data != NULL) {
                    value = data;
                    GlobalUnlock(dataHandle);
                }
            }
        }
        (void)selectedClipboardOperatingSystemInterface->CloseClipboard();
        return value;
    }

}

namespace {

    ClipboardOperatingSystemInterface clipboardWindowsInterface;

}

BOOL ClipboardOperatingSystemInterface::OpenClipboard(HWND hWndNewOwner) {
    return ::OpenClipboard(hWndNewOwner);
}

BOOL ClipboardOperatingSystemInterface::EmptyClipboard() {
    return ::EmptyClipboard();
}

BOOL ClipboardOperatingSystemInterface::IsClipboardFormatAvailable(UINT format) {
    return ::IsClipboardFormatAvailable(format);
}

HANDLE ClipboardOperatingSystemInterface::GetClipboardData(UINT uFormat) {
    return ::GetClipboardData(uFormat);
}

void ClipboardOperatingSystemInterface::SetClipboardData(
    UINT uFormat,
    HANDLE hMem
) {
    (void)::SetClipboardData(uFormat, hMem);
}

BOOL ClipboardOperatingSystemInterface::CloseClipboard() {
    return ::CloseClipboard();
}

ClipboardOperatingSystemInterface* selectedClipboardOperatingSystemInterface = &clipboardWindowsInterface;
