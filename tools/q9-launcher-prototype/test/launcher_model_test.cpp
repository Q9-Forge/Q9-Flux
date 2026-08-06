#include <cassert>
#include <string>

#include "launcher_model.h"

int main()
{
    LauncherModel empty({});
    assert(empty.configLabel() == "<leer>");
    assert(!empty.configSelectorVisible());
    assert(empty.themeName() == "Gold / Orange auf Schwarz");

    LauncherModel withConfigs({"emu.q9", "emu.hawk.q9"});
    withConfigs.toggleConfigSelector();
    assert(withConfigs.configSelectorVisible());
    assert(withConfigs.configCount() == 2);
    assert(withConfigs.configName(0) == "emu.q9");

    withConfigs.selectConfig(1);
    assert(withConfigs.configLabel() == "emu.hawk.q9");
    assert(withConfigs.startArgument() == "emu.hawk.q9");

    withConfigs.setTheme(LauncherTheme::GreenOnBlack);
    assert(withConfigs.themeName() == "Gruen auf Schwarz");
}
