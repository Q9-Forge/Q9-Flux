#define Uses_TApplication
#define Uses_TEvent
#define Uses_TRect
#define Uses_TView
#define Uses_TGroup
#define Uses_TDialog
#define Uses_TStaticText
#define Uses_TButton
#define Uses_TDeskTop
#define Uses_TDrawBuffer
#define Uses_TPalette
#define Uses_MsgBox
#include <tvision/tv.h>

#include <algorithm>
#include <dirent.h>
#include <string>
#include <vector>

#include "launcher_model.h"

extern TPoint shadowSize;
extern Boolean showMarkers;

enum {
    cmToggleConfig = 1000,
    cmStartEmulator,
    cmSettings,
    cmThemeWarm,
    cmThemeGreen,
    cmSelectConfigBase = 1100
};

static std::vector<std::string> findConfigs()
{
    std::vector<std::string> configs;
    DIR *dir = opendir(".");
    if (!dir) return configs;
    while (dirent *entry = readdir(dir)) {
        const std::string name(entry->d_name);
        if (name.size() > 3 && name.compare(name.size() - 3, 3, ".q9") == 0)
            configs.push_back(name);
    }
    closedir(dir);
    std::sort(configs.begin(), configs.end());
    return configs;
}

class ThemeSurface : public TView
{
public:
    ThemeSurface(const TRect &bounds, const LauncherModel &model) : TView(bounds), model_(model) {}

    void draw() override
    {
        const char main = model_.theme() == LauncherTheme::GreenOnBlack ? 0x0A : 0x0E;
        const char accent = model_.theme() == LauncherTheme::GreenOnBlack ? 0x02 : 0x06;
        for (short y = 0; y < size.y; ++y) line(y, "", 0x00);
        text(1, 25, "Q9 Flux Emulator - Motorola 68000", main);
        text(3, 29, "  QQQQ    9999", main);
        text(4, 29, " QQ  QQ  99  99", main);
        text(5, 29, " QQ  QQ   9999", main);
        text(6, 29, " QQ  QQ     99", main);
        text(7, 29, "  QQQQ    999 ", main);
        text(8, 34, "Q9  F L U X", accent);

        text(9, 7, "+     --------------------- EMU Config --------------------------+", accent);
        text(10, 7, "|     Config: " + model_.configLabel(), main);
        text(11, 7, "|                                                                  |", accent);
        if (model_.configSelectorVisible()) {
            text(12, 7, "| Verfuegbare Configs:                                             |", accent);
            for (std::size_t i = 0; i < model_.configCount(); ++i)
                text(13 + static_cast<short>(i * 2), 7, "|                                                                  |", accent);
            text(13 + static_cast<short>(model_.configCount() * 2), 7,
                 "+------------------------------------------------------------------+", accent);
        } else {
            text(12, 7, "+------------------------------------------------------------------+", accent);
        }
    }

private:
    const LauncherModel &model_;
    void line(short y, const std::string &value, char attr)
    {
        TDrawBuffer b;
        b.moveChar(0, ' ', attr, size.x);
        if (!value.empty()) b.moveStr(0, value.c_str(), attr);
        writeLine(0, y, size.x, 1, b);
    }
    void text(short y, short x, const std::string &value, char attr)
    {
        TDrawBuffer b;
        b.moveChar(0, ' ', 0x00, size.x);
        b.moveStr(x, value.c_str(), attr);
        writeLine(0, y, size.x, 1, b);
    }
};

class ThemedButton : public TButton
{
public:
    ThemedButton(const TRect &bounds, TStringView title, ushort command, ushort flags, LauncherTheme theme) :
        TButton(bounds, title, command, flags), theme_(theme) {}

    TPalette &getPalette() const override
    {
        // Index 8 is Turbo Vision's button shadow. Keep it black on black.
        static const char warmData[]  = { 0x0E, 0x06, 0x0E, 0x06, 0x06, 0x0E, 0x0E, 0x06, 0x00 };
        static const char greenData[] = { 0x0A, 0x02, 0x0A, 0x02, 0x02, 0x0A, 0x0A, 0x02, 0x00 };
        static TPalette warm(warmData, sizeof(warmData));
        static TPalette green(greenData, sizeof(greenData));
        return theme_ == LauncherTheme::GreenOnBlack ? green : warm;
    }
private:
    LauncherTheme theme_;
};

class ThemeDialog : public TDialog
{
public:
    explicit ThemeDialog(LauncherTheme theme) : TWindowInit(&TDialog::initFrame), TDialog(TRect(20, 6, 60, 18), "Einstellungen"), theme_(theme)
    {
        insert(new TStaticText(TRect(3, 2, 36, 5), "Theme:"));
        insert(new ThemedButton(TRect(5, 5, 35, 7), "Gold / Orange auf Schwarz", cmThemeWarm, bfDefault, theme_));
        insert(new ThemedButton(TRect(5, 8, 35, 10), "Gruen auf Schwarz", cmThemeGreen, bfNormal, theme_));
    }
    TPalette &getPalette() const override
    {
        static const char warmData[32] = { 0x06, 0x60, 0x0E, 0x06, 0x06, 0x0E, 0x06, 0x06,
                                            0x06, 0x0E, 0x06, 0x0E, 0x06, 0x06, 0x06, 0x06,
                                            0x06, 0x0E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
                                            0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06 };
        static const char greenData[32] = { 0x02, 0x20, 0x0A, 0x02, 0x02, 0x0A, 0x02, 0x02,
                                             0x02, 0x0A, 0x02, 0x0A, 0x02, 0x02, 0x02, 0x02,
                                             0x02, 0x0A, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
                                             0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 };
        static TPalette warm(warmData, sizeof(warmData));
        static TPalette green(greenData, sizeof(greenData));
        return theme_ == LauncherTheme::GreenOnBlack ? green : warm;
    }
    void handleEvent(TEvent &event) override
    {
        TDialog::handleEvent(event);
        if (event.what == evCommand && (event.message.command == cmThemeWarm || event.message.command == cmThemeGreen)) {
            endModal(event.message.command);
            clearEvent(event);
        }
    }
private:
    LauncherTheme theme_;
};

class Q9LauncherApp : public TApplication
{
public:
    explicit Q9LauncherApp(std::vector<std::string> configs) :
        TProgInit(&Q9LauncherApp::initStatusLine, &Q9LauncherApp::initMenuBar, &Q9LauncherApp::initDeskTop),
        model_(std::move(configs)), page_(0)
    {
        shadowSize.x = 0;
        shadowSize.y = 0;
        showMarkers = True;
        rebuildHome();
    }

    void handleEvent(TEvent &event) override
    {
        if (event.what == evKeyDown) {
            const char key = static_cast<char>(event.keyDown.charScan.charCode);
            if (key == '+' || key == 'c' || key == 'C') { model_.toggleConfigSelector(); rebuildHome(); clearEvent(event); return; }
            if (key == 'b' || key == 'B') { endModal(cmQuit); clearEvent(event); return; }
            if (key == 's' || key == 'S') { startPrototype(); clearEvent(event); return; }
            if (key == 'e' || key == 'E') { chooseTheme(); clearEvent(event); return; }
            if (model_.configSelectorVisible() && key >= '1' && key < static_cast<char>('1' + model_.configCount())) {
                model_.selectConfig(key - '1'); model_.toggleConfigSelector(); rebuildHome(); clearEvent(event); return;
            }
        }
        if (event.what == evCommand && event.message.command == cmQuit) {
            endModal(cmQuit);
            clearEvent(event);
            return;
        }
        TApplication::handleEvent(event);
        if (event.what != evCommand) return;
        const ushort command = event.message.command;
        if (command == cmToggleConfig) {
            model_.toggleConfigSelector(); rebuildHome(); clearEvent(event);
        } else if (command >= cmSelectConfigBase && command < cmSelectConfigBase + model_.configCount()) {
            model_.selectConfig(command - cmSelectConfigBase);
            if (model_.configSelectorVisible()) model_.toggleConfigSelector();
            rebuildHome(); clearEvent(event);
        } else if (command == cmSettings) {
            chooseTheme(); clearEvent(event);
        } else if (command == cmStartEmulator) {
            startPrototype(); clearEvent(event);
        }
    }

private:
    LauncherModel model_;
    TGroup *page_;
    static TMenuBar *initMenuBar(TRect) { return 0; }
    static TStatusLine *initStatusLine(TRect) { return 0; }
    static TDeskTop *initDeskTop(TRect r) { return new TDeskTop(r); }

    void startPrototype()
    {
        const std::string argument = model_.startArgument();
        if (argument.empty()) messageBox("Bitte zuerst eine EMU-Config auswaehlen.", mfWarning | mfOKButton);
        else messageBox(("Start wird im Prototyp nur simuliert.\n\nSpaeterer Aufruf:\n./build/native/q9.exe " + argument).c_str(), mfInformation | mfOKButton);
    }

    void chooseTheme()
    {
        const ushort result = executeDialog(new ThemeDialog(model_.theme()));
        if (result == cmThemeWarm) model_.setTheme(LauncherTheme::GoldOrangeOnBlack);
        if (result == cmThemeGreen) model_.setTheme(LauncherTheme::GreenOnBlack);
        rebuildHome();
    }

    void rebuildHome()
    {
        shadowSize.x = 0;
        shadowSize.y = 0;
        showMarkers = True;
        if (page_) { deskTop->remove(page_); destroy(page_); }
        page_ = new TGroup(deskTop->getExtent());
        page_->insert(new ThemeSurface(page_->getExtent(), model_));
        const short baseY = model_.configSelectorVisible() ? -1 : 22;
        page_->insert(new ThemedButton(TRect(8, 10, 13, 12), model_.configSelectorVisible() ? "−" : "+", cmToggleConfig, bfNormal, model_.theme()));
        if (model_.configSelectorVisible())
            for (std::size_t i = 0; i < model_.configCount(); ++i)
                page_->insert(new ThemedButton(TRect(10, 13 + static_cast<short>(i * 2), 36, 15 + static_cast<short>(i * 2)), model_.configName(i).c_str(), cmSelectConfigBase + i, bfNormal, model_.theme()));
        if (baseY >= 0) {
            page_->insert(new ThemedButton(TRect(12, baseY, 28, baseY + 2), "Starten", cmStartEmulator, bfDefault, model_.theme()));
            page_->insert(new ThemedButton(TRect(32, baseY, 52, baseY + 2), "Einstellungen", cmSettings, bfNormal, model_.theme()));
            page_->insert(new ThemedButton(TRect(56, baseY, 70, baseY + 2), "Beenden", cmQuit, bfNormal, model_.theme()));
        }
        deskTop->insert(page_);
    }
};

int main()
{
    Q9LauncherApp app(findConfigs());
    app.run();
    return 0;
}
