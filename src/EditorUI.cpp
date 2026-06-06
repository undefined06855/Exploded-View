#include "EditorUI.hpp"
#include "ExplodedLayerPopup.hpp"

bool HookedEditorUI::init(LevelEditorLayer* editor) {
    if (!EditorUI::init(editor)) return false;

    this->addEventListener(
        geode::KeybindSettingPressedEventV3(geode::Mod::get(), "explode-keybind"),
        [this](geode::Keybind const& keybind, bool down, bool repeat, double timestamp) {
            if (!down || repeat) return;
            if (cocos2d::CCScene::get()->getChildByType<ExplodedLayerPopup>(0)) return;

            auto popup = ExplodedLayerPopup::create();
            popup->m_noElasticity = true;
            popup->show();
        }
    );

    return true;
}

// prevent selecting anything while popup is open
bool HookedEditorUI::canSelectObject(GameObject* object) {
    if (ExplodedLayerPopup::get()) return false;
    else return EditorUI::canSelectObject(object);
}
