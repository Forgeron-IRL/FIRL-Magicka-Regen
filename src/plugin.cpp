#include <SKSE/SKSE.h>
#include <RE/Skyrim.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <SimpleIni.h>

#include <algorithm>

using namespace std::literals;

namespace {
    // ─────────────────────────────────────────────────────────────────────────
    //  Settings
    //
    //  Source of truth: the .ini managed by MCM Helper.
    //    - Shipped defaults: Data/MCM/Config/FIRLMagickaRegen/settings.ini
    //    - Player settings (written by the MCM): Data/MCM/Settings/FIRLMagickaRegen.ini
    //  Both are loaded in that order: the second overrides the first.
    //
    //  Target regeneration, per second:
    //    (max magicka × (fPercent + MagickaRate bonus) / 100 + fFlat) × MagickaRateMult / 100
    //  "MagickaRate bonus" = current value − base value: what perks / effects add to
    //  the rate. MagickaRateMult carries regeneration enchantments and potions
    //  (100 = normal). bMultAffectsFlat decides whether that multiplier also
    //  applies to the flat part.
    // ─────────────────────────────────────────────────────────────────────────
    struct Settings {
        bool  enabled = true;
        bool  player = true;
        bool  npcs = true;
        float percent = 3.0f;
        float flat = 5.0f;
        bool  multAffectsFlat = true;
        bool  respectDelay = false;

        static Settings& Get() {
            static Settings s;
            return s;
        }

        void Load() {
            CSimpleIniA ini;
            ini.SetUnicode();
            ini.LoadFile("Data/MCM/Config/FIRLMagickaRegen/settings.ini");
            ini.LoadFile("Data/MCM/Settings/FIRLMagickaRegen.ini");

            enabled = ini.GetBoolValue("General", "bEnabled", enabled);
            player = ini.GetBoolValue("General", "bPlayer", player);
            npcs = ini.GetBoolValue("General", "bNPCs", npcs);
            percent = static_cast<float>(ini.GetDoubleValue("Formula", "fPercent", percent));
            flat = static_cast<float>(ini.GetDoubleValue("Formula", "fFlat", flat));
            multAffectsFlat = ini.GetBoolValue("Formula", "bMultAffectsFlat", multAffectsFlat);
            respectDelay = ini.GetBoolValue("Formula", "bRespectDelay", respectDelay);

            SKSE::log::info(
                "Settings: enabled={} player={} npcs={} regen = {:.2f}% of max + {:.2f}/s "
                "(mult on flat={}, vanilla delay={})",
                enabled, player, npcs, percent, flat, multAffectsFlat, respectDelay);
        }
    };

    // fCombatMagickaRegenRateMult (0.33 in vanilla): what the game applies in
    // combat. Read once the game data is loaded.
    float g_combatMult = 0.33f;

    // Corrects an actor's magicka regeneration for this frame.
    //
    // Vanilla regeneration already runs inside Update (we run after it):
    //   max magicka × MagickaRate / 100 × MagickaRateMult / 100 (× fCombatMagickaRegenRateMult
    //   in combat), stopped during the delay that follows spending magicka.
    // Rather than disabling it (that would mean changing base values, i.e. the
    // save), we add or remove the difference with the target regeneration.
    void Apply(RE::Actor* a_actor, float a_delta) {
        const auto& s = Settings::Get();
        if (!s.enabled || a_delta <= 0.0f || a_actor->IsDead()) {
            return;
        }

        auto* av = a_actor->AsActorValueOwner();
        const float max = a_actor->GetActorValueMax(RE::ActorValue::kMagicka);
        if (max <= 0.0f) {
            return;
        }
        const float current = av->GetActorValue(RE::ActorValue::kMagicka);
        const bool  delayed = a_actor->GetRegenDelay(RE::ActorValue::kMagicka) > 0.0f;
        const float mult = (std::max)(av->GetActorValue(RE::ActorValue::kMagickaRateMult), 0.0f) / 100.0f;
        const float rate = av->GetActorValue(RE::ActorValue::kMagickaRate);
        const float bonus = rate - av->GetBaseActorValue(RE::ActorValue::kMagickaRate);

        float vanilla = 0.0f;
        if (!delayed && current < max) {
            vanilla = max * rate / 100.0f * mult;
            if (a_actor->IsInCombat()) {
                vanilla *= g_combatMult;
            }
        }

        float target = 0.0f;
        if (!(s.respectDelay && delayed)) {
            target = max * (s.percent + bonus) / 100.0f * mult + s.flat * (s.multAffectsFlat ? mult : 1.0f);
        }

        const float delta = (target - vanilla) * a_delta;
        if (delta > 0.0f) {
            if (current < max) {
                av->RestoreActorValue(RE::ActorValue::kMagicka, (std::min)(delta, max - current));
            }
        } else if (delta < 0.0f && current > 0.0f) {
            av->DamageActorValue(RE::ActorValue::kMagicka, (std::min)(-delta, current));
        }
    }

    // Hooks on Update (vfunc 0xAD), called every frame for every loaded actor.
    // PlayerCharacter and Character each have their own vtable.
    struct PlayerUpdate {
        static void Thunk(RE::PlayerCharacter* a_this, float a_delta) {
            func(a_this, a_delta);
            if (Settings::Get().player) {
                Apply(a_this, a_delta);
            }
        }
        static inline REL::Relocation<decltype(&Thunk)> func;

        static void Install() {
            REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_PlayerCharacter[0] };
            func = vtbl.write_vfunc(0xAD, Thunk);
            SKSE::log::info("PlayerCharacter::Update hook installed");
        }
    };

    struct CharacterUpdate {
        static void Thunk(RE::Character* a_this, float a_delta) {
            func(a_this, a_delta);
            if (Settings::Get().npcs) {
                Apply(a_this, a_delta);
            }
        }
        static inline REL::Relocation<decltype(&Thunk)> func;

        static void Install() {
            REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_Character[0] };
            func = vtbl.write_vfunc(0xAD, Thunk);
            SKSE::log::info("Character::Update hook installed");
        }
    };

    void InitLogger() {
        auto path = SKSE::log::log_directory();
        if (!path) {
            return;
        }
        *path /= "FIRLMagickaRegen.log"sv;

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto logger = std::make_shared<spdlog::logger>("global", std::move(sink));
        logger->set_level(spdlog::level::info);
        logger->flush_on(spdlog::level::info);
        spdlog::set_default_logger(std::move(logger));
        spdlog::set_pattern("[%H:%M:%S] [%l] %v");
    }

    // Reloads the settings when the player closes the menu (the MCM lives in the
    // "Journal Menu"). MCM Helper writes the INI as soon as a value changes, so
    // the INI is up to date when the menu closes → applied immediately.
    class MenuSink final : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static MenuSink* GetSingleton() {
            static MenuSink singleton;
            return &singleton;
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::MenuOpenCloseEvent* a_event,
            RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
        {
            if (a_event && !a_event->opening && a_event->menuName == "Journal Menu"sv) {
                Settings::Get().Load();
            }
            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        MenuSink() = default;
    };

    void OnMessage(SKSE::MessagingInterface::Message* a_msg) {
        if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
            if (auto* gmst = RE::GameSettingCollection::GetSingleton()) {
                if (auto* setting = gmst->GetSetting("fCombatMagickaRegenRateMult")) {
                    g_combatMult = setting->GetFloat();
                }
            }
            SKSE::log::info("fCombatMagickaRegenRateMult = {:.2f}", g_combatMult);

            if (REX::W32::GetModuleHandleW(L"RegenOverhaul.dll")) {
                SKSE::log::warn("Regeneration Overhaul is loaded: its hook changes vanilla regeneration, "
                                "so this plugin's correction will be wrong. Disable it.");
            }

            if (auto* ui = RE::UI::GetSingleton()) {
                ui->AddEventSink(MenuSink::GetSingleton());
            }
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    SKSE::Init(a_skse);
    InitLogger();
    SKSE::log::info("FIRLMagickaRegen loaded (runtime {})", REL::Module::get().version().string());

    Settings::Get().Load();
    PlayerUpdate::Install();
    CharacterUpdate::Install();

    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
