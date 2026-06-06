#pragma once
#include <Geode/modify/EditorUI.hpp>

class $modify(HookedEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editor);
    bool canSelectObject(GameObject* object);
};
