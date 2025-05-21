// src/addon.cpp
#include <napi.h>

#ifdef _WIN32
#include <windows.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include <thumbcache.h>
#include <memory>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Gdi32.lib")

using namespace Gdiplus;

#ifndef BHID_ThumbnailHandler
extern "C" const GUID BHID_ThumbnailHandler = {
    0x7b2e650a, 0x8e20, 0x4f4a,
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

void SaveBitmapAsPNG(HBITMAP hBitmap, const std::wstring& outputPath) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Ok) return;

    BITMAP bm;
    if (!GetObject(hBitmap, sizeof(BITMAP), &bm)) return;

    Bitmap bitmap(bm.bmWidth, bm.bmHeight, PixelFormat32bppARGB);
    BitmapData bmpData;
    Rect rect(0, 0, bm.bmWidth, bm.bmHeight);

    if (bitmap.LockBits(&rect, ImageLockModeWrite, PixelFormat32bppARGB, &bmpData) == Ok) {
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = bm.bmWidth;
        bmi.bmiHeader.biHeight = -bm.bmHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        HDC hdc = GetDC(nullptr);
        GetDIBits(hdc, hBitmap, 0, bm.bmHeight, bmpData.Scan0, &bmi, DIB_RGB_COLORS);
        ReleaseDC(nullptr, hdc);

        bitmap.UnlockBits(&bmpData);
    }

    CLSID clsidPng;
    CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &clsidPng);
    bitmap.Save(outputPath.c_str(), &clsidPng, nullptr);

    GdiplusShutdown(gdiplusToken);
}

void getImage(const Napi::CallbackInfo& info, bool useThumbnail) {
    Napi::Env env = info.Env();
    if (info.Length() != 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsNumber()) {
        Napi::TypeError::New(env, "Expected (inputPath: string, outputPath: string, size: number)").ThrowAsJavaScriptException();
        return;
    }

    std::wstring inputPath = std::wstring(info[0].As<Napi::String>().Utf8Value().begin(), info[0].As<Napi::String>().Utf8Value().end());
    std::wstring outputPath = std::wstring(info[1].As<Napi::String>().Utf8Value().begin(), info[1].As<Napi::String>().Utf8Value().end());
    int size = info[2].As<Napi::Number>().Int32Value();

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Ok) {
        Napi::Error::New(env, "Failed to initialize GDI+").ThrowAsJavaScriptException();
        return;
    }

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
        GdiplusShutdown(gdiplusToken);
        Napi::Error::New(env, "Failed to initialize COM").ThrowAsJavaScriptException();
        return;
    }

    HBITMAP hBitmap = nullptr;
    HRESULT hr = useThumbnail
        ? GetThumbnailImage(inputPath, size, hBitmap)
        : GetIconImage(inputPath, size, hBitmap);

    if (SUCCEEDED(hr) && hBitmap) {
        SaveBitmapAsPNG(hBitmap, outputPath);
        DeleteObject(hBitmap);
    } else {
        Napi::Error::New(env, "Failed to retrieve image").ThrowAsJavaScriptException();
    }

    CoUninitialize();
    GdiplusShutdown(gdiplusToken);
}

Napi::Value getIcon(const Napi::CallbackInfo& info) {
    getImage(info, false);
    return info.Env().Undefined();
}

Napi::Value getThumbnail(const Napi::CallbackInfo& info) {
    getImage(info, true);
    return info.Env().Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("getIcon", Napi::Function::New(env, getIcon));
    exports.Set("getThumbnail", Napi::Function::New(env, getThumbnail));
    return exports;
}

NODE_API_MODULE(winicon, Init)

#else

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function stub = Napi::Function::New(env, [](const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        Napi::TypeError::New(env, "winicon only works on Windows.").ThrowAsJavaScriptException();
    });

    exports.Set("getIcon", stub);
    exports.Set("getThumbnail", stub);
    return exports;
}

NODE_API_MODULE(winicon, Init)

#endif
