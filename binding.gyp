{
  "targets": [
    {
      "target_name": "winicon",
      "sources": ["src/winicon.cc"],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "defines": ["NAPI_CPP_EXCEPTIONS"],
      "conditions": [
        ["OS=='win'", {
          "defines": ["WIN32"],
          "libraries": [
            "Ole32.lib",
            "Shlwapi.lib",
            "Gdiplus.lib",
            "Gdi32.lib"
          ]
        }]
      ],
      "msvs_settings": {
        "VCCLCompilerTool": {
          "AdditionalOptions": ["/std:c++17"]
        }
      },
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
    }
  ]
}