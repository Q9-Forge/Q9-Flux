#ifndef Q9_LAUNCHER_MODEL_H
#define Q9_LAUNCHER_MODEL_H

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

enum class LauncherTheme { GoldOrangeOnBlack, GreenOnBlack };

class LauncherModel {
public:
    explicit LauncherModel(std::vector<std::string> configs) : configs_(std::move(configs)) {}

    void toggleConfigSelector() { selectorVisible_ = !selectorVisible_; }
    bool configSelectorVisible() const { return selectorVisible_; }
    std::size_t configCount() const { return configs_.size(); }
    const std::string &configName(std::size_t index) const { return configs_.at(index); }

    void selectConfig(std::size_t index) {
        if (index < configs_.size()) selected_ = index;
    }
    void setTheme(LauncherTheme theme) { theme_ = theme; }
    LauncherTheme theme() const { return theme_; }
    std::string themeName() const {
        return theme_ == LauncherTheme::GreenOnBlack ? "Gruen auf Schwarz" : "Gold / Orange auf Schwarz";
    }
    std::string configLabel() const { return selected_ < configs_.size() ? configs_[selected_] : "<leer>"; }
    std::string startArgument() const { return selected_ < configs_.size() ? configs_[selected_] : ""; }

private:
    std::vector<std::string> configs_;
    std::size_t selected_ = static_cast<std::size_t>(-1);
    bool selectorVisible_ = false;
    LauncherTheme theme_ = LauncherTheme::GoldOrangeOnBlack;
};

#endif
