{
  "targets": [
    {
      "target_name": "@filemap/winicon",
      "sources": ["src/winicon.cc"],
      "include_dirs": [
        "<!(node -p \"require('node-addon-api').include\")"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "link_settings": {
        "libraries": []
      },
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
      ]
    }
  ]
}