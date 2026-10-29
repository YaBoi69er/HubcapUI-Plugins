// Builds the production parser in-process; run-catalog-parser-tests.ps1 compiles and runs it.
#include "../src/feature_engine.cpp"

#include <cassert>

std::string hubcap_plugins_catalog_json() { return "[]"; }
std::string hubcap_plugins_set_enabled(const std::string&, bool) { return {}; }
std::string hubcap_plugins_handle(const std::string&, const std::string&) { return {}; }
extern "C" void hubcap_runtime_log(const char*) {}

namespace {

const char* const fixture = R"WIKI(# List

| Name | Maintainer | Links | Status |
| :--- | :--- | :--- | :---: |
| Example Snapshot Game | Someone | [![Snapshot](https://img.shields.io/x)](https://someone.github.io/renodx/renodx-example.addon64) | :white_check_mark: |
| Nexus Only Game | Someone | [![Nexus Mods](https://img.shields.io/x)](https://www.nexusmods.com/game/mods/1) | :construction: |

## Multi-Game Mods

### Unreal Engine Extended [![Snapshot](https://img.shields.io/x)](https://marat569.github.io/renodx/renodx-ue-extended.addon64)

> [!TIP]
> Helper: [![Snapshot](https://img.shields.io/x)](https://github.com/clshortfuse/renodx/releases/download/snapshot/renodx-dlssfix.addon64)

| Name | Status | Notes |
| :--- | :---: | :--- |
| STAR WARS: Galactic Racer™ | :white_check_mark: | Native HDR |
| Abiotic Factor | :white_check_mark: | Engine.ini |
| BLACKTAIL | :white_check_mark: | Upgrade Path: On |
| Deep Rock Galactic: Rogue Core | :white_check_mark: | Upgrade Path: On, `R10G10B10A2_UNORM` `Output Size`. Native HDR is broken. |
| Shared Game | :white_check_mark: | Native HDR |

<details>
<summary>Legacy Unreal Mod</summary>

 ### Unreal Engine [![Snapshot](https://img.shields.io/x)](https://clshortfuse.github.io/renodx/renodx-unrealengine.addon64)

> [!TIP]
> **DLSSFIX** [![Snapshot](https://img.shields.io/x)](https://github.com/clshortfuse/renodx/releases/download/snapshot/renodx-dlssfix.addon64)

| Name | Status | Notes |
| :--- | :---: | :--- |
| Ashen | :white_check_mark: | `B8G8R8A8_TYPELESS` `Output Size`</details> |
| Shared Game | :white_check_mark: | Legacy notes |
</details>

### Unity Engine
64-bit: [![S](https://img.shields.io/x)](https://notvoosh.github.io/renodx-unity/renodx-unityengine.addon64) | 32-bit: [![S](https://img.shields.io/x)](https://notvoosh.github.io/renodx-unity/renodx-unityengine.addon32)

| Name | Status | Notes |
| :--- | :---: | :--- |
| Unity Game | :white_check_mark: | Enable Swapchain Proxy |

### Source Engine
32-bit: [![S](https://img.shields.io/x)](https://github.com/CommandGenius/renodx/releases/download/snapshot/renodx-sourceengine.addon32)

| Name | Status | Notes |
| :--- | :---: | :--- |
| Garry's Mod | :white_check_mark: | Both 32 AND 64-bit. |

### Untrusted Unreal [![S](https://img.shields.io/x)](https://evil.example.com/renodx-unrealengine.addon64)

| Name | Status | Notes |
| :--- | :---: | :--- |
| Untrusted Game | :white_check_mark: | Native HDR |

# Related Mods

| Name | Maintainer | Links |
| :--- | :--- | :--- |
| Related Game | Someone | [![Nexus Mods](https://img.shields.io/x)](https://www.nexusmods.com/related/mods/2) |

# Deprecated mods

| Name | Maintainer | Links |
| :--- | :--- | :--- |
| Abiotic Factor | Someone | [![Snapshot](https://img.shields.io/x)](https://someone.github.io/renodx/renodx-abiotic.addon64) |
)WIKI";

bool contains(const std::vector<std::string>& steps, const std::string& text)
{
    for (const auto& step : steps) if (step.find(text) != std::string::npos) return true;
    return false;
}

RenoMod find(const std::vector<RenoMod>& catalog, const std::string& name, bool expect_found = true)
{
    RenoMod mod;
    const bool found = find_reno_mod_in(catalog, name, mod);
    if (found != expect_found) {
        std::printf("Unexpected match result for %s\n", name.c_str());
        std::exit(1);
    }
    return mod;
}

void fixture_tests()
{
    const auto catalog = parse_reno_catalog(fixture);

    const auto snapshot = find(catalog, "Example Snapshot Game");
    assert(!snapshot.generic && snapshot.snapshot_url == "https://someone.github.io/renodx/renodx-example.addon64");
    assert(find(catalog, "Nexus Only Game").construction);

    const auto racer = find(catalog, "STAR WARS: Galactic Racer");
    assert(racer.generic && !racer.legacy && racer.engine == "Unreal Engine");
    assert(racer.snapshot_url == "https://marat569.github.io/renodx/renodx-ue-extended.addon64");
    assert(racer.profile == "renodx-ue-extended");
    assert(racer.settings.at("Set_Path") == 0 && racer.settings.at("SettingsMode") == 2);
    assert(racer.engine_ini.empty() && contains(racer.user_steps, "game's display settings"));
    assert(contains(racer.auto_steps, "Upgrade Path to Off") && contains(racer.fallback_steps, "Upgrade Path"));

    const auto abiotic = find(catalog, "Abiotic Factor");
    assert(abiotic.generic && abiotic.settings.at("Set_Path") == 0);
    assert(abiotic.engine_ini.size() == 6 && contains(abiotic.auto_steps, "Engine.ini"));
    assert(contains(abiotic.fallback_steps, "r.HDR.EnableHDROutput=1"));

    const auto blacktail = find(catalog, "BLACKTAIL");
    assert(blacktail.settings.at("Set_Path") == 1);

    const auto rogue = find(catalog, "Deep Rock Galactic: Rogue Core");
    assert(rogue.settings.at("Set_Path") == 1 && rogue.settings.at("Upgrade_R10G10B10A2_UNORM") == 1);
    assert(contains(rogue.user_steps, "Keep the game's own HDR setting turned off"));
    assert(contains(rogue.auto_steps, "Apply 1 RenoDX resource upgrade setting"));

    const auto ashen = find(catalog, "Ashen");
    assert(ashen.generic && ashen.legacy && ashen.snapshot_url == "https://clshortfuse.github.io/renodx/renodx-unrealengine.addon64");
    assert(ashen.settings.count("Set_Path") == 0 && ashen.settings.at("Upgrade_B8G8R8A8_TYPELESS") == 1);

    const auto shared = find(catalog, "Shared Game");
    assert(!shared.legacy && shared.profile == "renodx-ue-extended");

    const auto unity = find(catalog, "Unity Game");
    assert(unity.engine == "Unity Engine" && unity.snapshot_url_32 == "https://notvoosh.github.io/renodx-unity/renodx-unityengine.addon32");
    assert(unity.settings.at("Use_Swapchain_Proxy") == 1);

    find(catalog, "Garry's Mod", false);
    find(catalog, "Untrusted Game", false);
    find(catalog, "Related Game", false);

    assert(!trusted_generic_addon_url("https://evil.example.com/renodx-unrealengine.addon64"));
    assert(!trusted_generic_addon_url("https://github.com/someone/renodx/raw/main/renodx-unrealengine.addon64"));
    assert(!trusted_generic_addon_url("https://someone.github.io/renodx/other.addon64"));
    assert(!generic_addon_matches_engine("Unreal Engine", "https://github.com/clshortfuse/renodx/releases/download/snapshot/renodx-dlssfix.addon64"));
}

std::string read_text(const std::wstring& path)
{
    std::ifstream file(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void write_text(const std::wstring& path, const std::string& text)
{
    std::ofstream(path, std::ios::binary) << text;
}

bool read_only(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_READONLY);
}

// Runs against a temporary LOCALAPPDATA and game folder; never touches real game config.
void engine_ini_tests()
{
    wchar_t temp[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, temp);
    const std::wstring root = join(temp, L"hubcap-reno-engine-ini-test");
    delete_directory_tree(root);
    const std::wstring app_data = join(root, L"LocalAppData");
    const std::wstring game = join(root, L"Game");
    SHCreateDirectoryExW(nullptr, app_data.c_str(), nullptr);
    SHCreateDirectoryExW(nullptr, join(game, L"Engine\\Binaries\\Win64").c_str(), nullptr);
    SHCreateDirectoryExW(nullptr, join(game, L"Racer\\Binaries\\Win64").c_str(), nullptr);
    SetEnvironmentVariableW(L"LOCALAPPDATA", app_data.c_str());
    const auto settings = unreal_engine_ini_settings("Engine.ini");
    std::string error;

    // Never launched: default UE5 folder, created by Hubcap and removed again on undo.
    auto location = locate_unreal_config(game);
    assert(!location.launched && location.directory == join(app_data, L"Racer\\Saved\\Config\\Windows"));
    assert(apply_engine_ini("1", location.directory, settings, true, error));
    const std::wstring created = join(location.directory, L"Engine.ini");
    assert(read_only(created) && read_text(created).find("r.HDR.EnableHDROutput=1") != std::string::npos);
    assert(read_text(created).find("[/Script/Engine.RendererSettings]") != std::string::npos);
    assert(read_engine_ini_state(engine_ini_state_path("1", L".ini"), L"Pending") == L"1");
    assert(undo_engine_ini("1") && GetFileAttributesW(created.c_str()) == INVALID_FILE_ATTRIBUTES);

    // Launched UE4 game: follow its WindowsNoEditor folder, keep user lines, restore the original.
    const std::wstring ue4 = join(app_data, L"Racer\\Saved\\Config\\WindowsNoEditor");
    SHCreateDirectoryExW(nullptr, ue4.c_str(), nullptr);
    write_text(join(ue4, L"GameUserSettings.ini"), "[x]\r\n");
    const std::string original = "[Core.System]\r\nPaths=../../Content\r\n";
    write_text(join(ue4, L"Engine.ini"), original);
    location = locate_unreal_config(game);
    assert(location.launched && location.directory == ue4);
    assert(apply_engine_ini("2", location.directory, settings, false, error));
    const std::string merged = read_text(join(ue4, L"Engine.ini"));
    assert(merged.find("Paths=../../Content") != std::string::npos && merged.find("r.AllowHDR=1") != std::string::npos);
    assert(apply_engine_ini("2", location.directory, settings, false, error));
    assert(undo_engine_ini("2") && read_text(join(ue4, L"Engine.ini")) == original && !read_only(join(ue4, L"Engine.ini")));

    // "Only add" notes narrow the block to the named variables.
    const auto narrowed = unreal_engine_ini_settings("Engine.ini, Only add r.HDR.EnableHDROutput=1 to Engine.ini.");
    assert(narrowed.size() == 1 && narrowed[0].key == "r.HDR.EnableHDROutput" && narrowed[0].value == "1");

    delete_directory_tree(root);
}

void live_tests(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    const std::string markdown((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const auto catalog = parse_reno_catalog(markdown);
    size_t generic = 0, unreal = 0;
    for (const auto& mod : catalog) {
        generic += mod.generic;
        unreal += mod.generic && mod.engine == "Unreal Engine" && !mod.legacy;
    }
    std::printf("Live wiki: %zu entries, %zu generic, %zu Unreal Engine Extended.\n", catalog.size(), generic, unreal);
    assert(unreal > 50);
    const auto racer = find(catalog, "STAR WARS: Galactic Racer");
    assert(racer.generic && !racer.legacy && racer.engine == "Unreal Engine");
    assert(find(catalog, "Black Myth: Wukong").generic);
    find(catalog, "Garry's Mod", false);
}

} // namespace

int main(int argc, char** argv)
{
    fixture_tests();
    engine_ini_tests();
    if (argc > 1) live_tests(argv[1]);
    std::puts("RenoDX catalog parser tests passed.");
    return 0;
}
