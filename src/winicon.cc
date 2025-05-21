// src/addon.cpp
#include <napi.h>

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include <thumbcache.h>
#include <memory>
#include <exception>
#include <fstream>
#include <algorithm>
#include <iostream>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Gdi32.lib")

using namespace Gdiplus;

#ifndef BHID_ThumbnailHandler
extern "C" const GUID BHID_ThumbnailHandler = {
    0x72b2e650a, 0x8e20, 0x4f4a,
    { 0xb0, 0x9e, 0x65, 0x97, 0xaf, 0xde, 0x3c, 0x94 }
};
#endif

HRESULT GetIconImage(const std::wstring& filePath, int size, HBITMAP& hBitmap) {
    IShellItem* pShellItem = nullptr;
    HRESULT hr = SHCreateItemFromParsingName(filePath.c_str(), nullptr, IID_PPV_ARGS(&pShellItem));
    if (FAILED(hr)) return hr;

    IShellItemImageFactory* pImageFactory = nullptr;
    hr = pShellItem->QueryInterface(IID_PPV_ARGS(&pImageFactory));
    if (FAILED(hr)) {
        pShellItem->Release();
        return hr;
    }

    SIZE sizeStruct = { size, size };
    hr = pImageFactory->GetImage(sizeStruct, SIIGBF_BIGGERSIZEOK | SIIGBF_ICONONLY | SIIGBF_RESIZETOFIT, &hBitmap);

    pImageFactory->Release();
    pShellItem->Release();
    return hr;
}

HRESULT GetThumbnailImage(const std::wstring& filePath, int size, HBITMAP& hBitmap) {
    IShellItem* psi = nullptr;
    HRESULT hr = SHCreateItemFromParsingName(filePath.c_str(), nullptr, IID_PPV_ARGS(&psi));
    if (FAILED(hr)) return hr;

    IThumbnailProvider* pThumbProvider = nullptr;
    hr = psi->BindToHandler(nullptr, BHID_ThumbnailHandler, IID_PPV_ARGS(&pThumbProvider));
    if (FAILED(hr)) {
        psi->Release();
        return hr;
    }

    WTS_ALPHATYPE wtsAlpha;
    hr = pThumbProvider->GetThumbnail(size, &hBitmap, &wtsAlpha);

    pThumbProvider->Release();
    psi->Release();
    return hr;
}

Napi::Value getImageBuffer(const Napi::CallbackInfo& info, bool useThumbnail) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (inputPath: string, size: number)").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::wstring inputPath = std::wstring(info[0].As<Napi::String>().Utf8Value().begin(), info[0].As<Napi::String>().Utf8Value().end());
    int size = info[1].As<Napi::Number>().Int32Value();

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
        Napi::Error::New(env, "Failed to initialize COM").ThrowAsJavaScriptException();
        return env.Null();
    }

    HBITMAP hBitmap = nullptr;
    Napi::Value result = env.Null();
    try {
        HRESULT hr = useThumbnail
            ? GetThumbnailImage(inputPath, size, hBitmap)
            : GetIconImage(inputPath, size, hBitmap);

        if (SUCCEEDED(hr) && hBitmap) {
            BITMAP bmp;
            if (GetObject(hBitmap, sizeof(BITMAP), &bmp) && bmp.bmWidth > 0 && bmp.bmHeight > 0) {
                std::wcerr << L"Bitmap: " << bmp.bmWidth << L"x" << bmp.bmHeight << std::endl;

                BITMAPINFO bmi = {};
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = bmp.bmWidth;
                bmi.bmiHeader.biHeight = -bmp.bmHeight;
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 32;
                bmi.bmiHeader.biCompression = BI_RGB;

                int bmpSize = bmp.bmWidth * 4 * bmp.bmHeight;
                std::unique_ptr<BYTE[]> pixels(new BYTE[bmpSize]);

                HDC hMemDC = CreateCompatibleDC(nullptr);
                if (hMemDC) {
                    int lines = GetDIBits(hMemDC, hBitmap, 0, bmp.bmHeight, pixels.get(), &bmi, DIB_RGB_COLORS);
                    if (lines > 0) {
                        std::wcerr << L"GetDIBits returned " << lines << L" scanlines." << std::endl;
                        result = Napi::Buffer<BYTE>::Copy(env, pixels.get(), bmpSize);
                    } else {
                        std::wcerr << L"GetDIBits failed or returned zero lines." << std::endl;
                        Napi::Error::New(env, "GetDIBits failed").ThrowAsJavaScriptException();
                    }
                    DeleteDC(hMemDC);
                } else {
                    Napi::Error::New(env, "CreateCompatibleDC failed").ThrowAsJavaScriptException();
                }
            } else {
                std::wcerr << L"Invalid bitmap dimensions." << std::endl;
                Napi::Error::New(env, "Invalid bitmap").ThrowAsJavaScriptException();
            }
            DeleteObject(hBitmap);
        } else {
            std::wcerr << L"Failed to get icon/thumbnail. HRESULT=" << hr << std::endl;
            Napi::Error::New(env, "Failed to retrieve image").ThrowAsJavaScriptException();
        }
    } catch (...) {
        if (hBitmap) DeleteObject(hBitmap);
        Napi::Error::New(env, "Native exception in image processing").ThrowAsJavaScriptException();
    }

    CoUninitialize();
    return result;
}

Napi::Value getIconBuffer(const Napi::CallbackInfo& info) {
    return getImageBuffer(info, false);
}

Napi::Value getThumbnailBuffer(const Napi::CallbackInfo& info) {
    return getImageBuffer(info, true);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("getIconBuffer", Napi::Function::New(env, getIconBuffer));
    exports.Set("getThumbnailBuffer", Napi::Function::New(env, getThumbnailBuffer));
    return exports;
}

NODE_API_MODULE(winicon, Init)

#else

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function stub = Napi::Function::New(env, [](const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        Napi::TypeError::New(env, "winicon only works on Windows.").ThrowAsJavaScriptException();
    });

    exports.Set("getIconBuffer", stub);
    exports.Set("getThumbnailBuffer", stub);
    return exports;
}

NODE_API_MODULE(winicon, Init)

#endif
