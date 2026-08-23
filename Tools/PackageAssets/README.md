# Package assets

These tracked defaults make `Tools/package.ps1` reproducible from a fresh checkout:

- `default_scenario.json` is copied to `Resources/Logs/scenario.json` in the package.
- `default_imgui.ini` is copied to `imgui.ini` beside `TestApp.exe`.

The application may overwrite the packaged copies at runtime. Development recordings under
`Resources/Logs/` and local `imgui.ini` files remain ignored so running the demo does not dirty the
working tree.
