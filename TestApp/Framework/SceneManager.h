#pragma once
#include <vector>
#include <string>
#include <functional>

// Lightweight singleton that holds the scene list and wires up the scene browser buttons.
// App.cpp registers scenes + a switch callback; each scene's RenderForward() calls SwitchTo().
class SceneManager
{
public:
    static SceneManager& Get() { static SceneManager s; return s; }

    struct Entry { std::string name; };

    void RegisterScenes(std::vector<std::string> names, std::function<void(int)> onSwitch)
    {
        entries.clear();
        for (auto& n : names)
            entries.push_back({ std::move(n) });
        switchCallback = std::move(onSwitch);
    }

    void SwitchTo(int index)
    {
        if (switchCallback && index != currentIndex)
            switchCallback(index);
    }

    void SetCurrentIndex(int i) { currentIndex = i; }
    int  GetCurrentIndex()  const { return currentIndex; }
    const std::vector<Entry>& GetEntries() const { return entries; }

private:
    SceneManager() = default;
    std::vector<Entry>        entries;
    std::function<void(int)>  switchCallback;
    int                       currentIndex = -1;
};
