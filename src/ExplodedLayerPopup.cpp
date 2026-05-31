#include "ExplodedLayerPopup.hpp"
#include <geode.devtools/include/API.hpp>

ExplodedLayerPopup* ExplodedLayerPopup::create() {
    auto ret = new ExplodedLayerPopup;
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool ExplodedLayerPopup::init() {
    auto winSize = cocos2d::CCDirector::get()->getWinSize();
    if (!Popup::init(winSize)) return false;

    m_editor = LevelEditorLayer::get();

    m_lier = RenderNodeLier::create([this] { this->drawObjectLayers(); });

    m_3dNode = ThreeDeeAPI::ThirdDimensionNode::create();
    m_3dNode->setID("3d-node");
    m_3dNode->setCameraAim({ 91.f, 6.f, -172.f });
    m_3dNode->setCameraPosition({ 242.f, 115.f, 57.f });
    m_3dNode->setCameraFOV(70.f);
    m_3dNode->setFlipY(true);

    m_layerLabel = cocos2d::CCLabelBMFont::create("...", "bigFont.fnt");
    m_layerLabel->setID("current-layer-label");
    m_layerLabel->setPosition({ winSize.width / 2.f, winSize.height - 30.f });
    m_mainLayer->addChild(m_layerLabel);

    m_layerGap = 60.f;
    m_layerOffset = 0.f;

    std::set<int> layers;
    for (auto object : geode::cocos::CCArrayExt<GameObject*>(m_editor->m_objects)) {
        layers.insert(object->m_editorLayer);
        if (object->m_editorLayer2 != 0) layers.insert(object->m_editorLayer2);
    }

    for (auto layer : layers) {
        auto renderNode = alpha::ui::RenderNode::create(m_lier, /* fullscreen */ false);
        // do this here and not automatically because we need the texture later
        // else we get funny cc_2x2_white_square
        renderNode->initFBO();

        auto plane = ThreeDeeAPI::ThirdDimensionObject::create();
        plane->setTag(layer); // for devtools only
        auto ret = plane->loadObject("plane.obj"_spr, /* flip uvs */ true);
        if (ret.isErr()) {
            geode::log::error("error loading: {}", ret.unwrapErr());
            return true;
        }

        auto winSize = cocos2d::CCDirector::get()->getWinSize();
        plane->setScaleX(winSize.width);
        plane->setScaleY(winSize.height);

        plane->setTexture(renderNode->getTexture());

        m_3dNode->addChild(plane);

        m_layers[layer] = { plane, renderNode };
    }

    m_mainLayer->addChildAtPosition(m_3dNode, geode::Anchor::Center);
    m_closeBtn->setLayoutOptions(
        geode::AnchorLayoutOptions::create()
            ->setOffset({ 25.f, -25.f })
            ->setAnchor(geode::Anchor::TopLeft)
    );
    m_buttonMenu->updateLayout();

    m_bgSprite->removeFromParent();

    this->setOpacity(0);
    this->runAction(cocos2d::CCSequence::createWithTwoActions(
        cocos2d::CCFadeIn::create(1.f),
        geode::cocos::CallFuncExt::create([this] {
            m_oldEditorMode = m_editor->m_editorUI->m_selectedMode;
            m_editor->m_editorUI->m_selectedMode = 3; // edit mode
        })
    ));

    this->scheduleUpdate();
    this->setMouseEnabled(true);

    return true;
}

void ExplodedLayerPopup::onClose(cocos2d::CCObject* sender) {
    Popup::onClose(sender);
    m_editor->m_editorUI->m_selectedMode = m_oldEditorMode;
}

bool ExplodedLayerPopup::ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    return m_editor->m_editorUI->ccTouchBegan(touch, event);
}

void ExplodedLayerPopup::ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    m_editor->m_editorUI->ccTouchMoved(touch, event);
}

void ExplodedLayerPopup::ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    m_editor->m_editorUI->ccTouchEnded(touch, event);
}

void ExplodedLayerPopup::ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    m_editor->m_editorUI->ccTouchCancelled(touch, event);
}

void ExplodedLayerPopup::scrollWheel(float y, float x) {
    m_layerOffset += y;
}

void ExplodedLayerPopup::update(float dt) {
    float offset = 0.f;
    for (auto [_, layer] : m_layers) {
        layer.object->setPositionZ(offset + m_layerOffset);
        layer.object->setZOrder(m_layers.size() - (-offset / m_layerGap));

        // -z is away from the camera
        offset -= m_layerGap;
    }
}

#define HIDE(node, variableLabel) \
    auto was##variableLabel##Visible = true; \
    if (node) { \
        was##variableLabel##Visible = node->isVisible(); \
        node->setVisible(false); \
    } \
    do {} while(false)

#define UNHIDE(node, variableLabel) \
    if (node && was##variableLabel##Visible) { \
        node->setVisible(true); \
    } \
    do {} while(false)

void ExplodedLayerPopup::draw() {
    HIDE(m_editor->m_background, Background);
    HIDE(m_editor->m_groundLayer, GroundLayer);
    HIDE(m_editor->m_groundLayer2, GroundLayer2);
    HIDE(m_editor->m_middleground, Middleground);
    HIDE(m_editor->m_drawGridLayer, DrawGridLayer);

    std::vector<bool> origVisibilities;
    origVisibilities.resize(m_editor->m_activeObjectsCount, false);
    for (int i = 0; i < m_editor->m_activeObjectsCount; i++) {
        auto object = m_editor->m_activeObjects[i];
        origVisibilities[i] = object->isVisible();
    }

    // taken from rewind
    m_cachedGameplayNodes = {
        m_editor->m_objectParent,
        m_editor->m_inShaderParent,
        m_editor->m_shaderLayer,
        m_editor->m_aboveShaderParent,
        m_editor->m_objectLayer,
        m_editor->m_inShaderObjectLayer,
        m_editor->m_aboveShaderObjectLayer,
        m_editor->m_uiTriggerUI
    };

    std::sort(m_cachedGameplayNodes.begin(), m_cachedGameplayNodes.end(), [](cocos2d::CCNode* left, cocos2d::CCNode* right) {
        return left->getZOrder() < right->getZOrder();
    });

    std::erase_if(m_cachedGameplayNodes, [this](cocos2d::CCNode* node) {
        return node->getParent() != m_editor;
    });

    for (auto& [ layer, data ] : m_layers) {
        // remember -z is out of the camera and +z is towards the camera
        data.object->setVisible(false);
        if (data.object->getPositionZ() > 0.f) continue; // planes too close
        if (data.object->getPositionZ() < -m_layerGap * 7.f) continue; // planes too far
        data.object->setVisible(true);

        bool anyObjects = false;
        for (int i = 0; i < m_editor->m_activeObjectsCount; i++) {
            auto object = m_editor->m_activeObjects[i];
            auto visible = object->m_editorLayer == layer || (object->m_editorLayer2 != 0 && object->m_editorLayer2 == layer);
            if (visible) anyObjects = true;
            object->setVisible(visible);
        }

        if (!anyObjects) continue;

        m_currentLayer = layer;

        data.renderNode->render();
    }

    for (int i = 0; i < m_editor->m_activeObjectsCount; i++) {
        auto object = m_editor->m_activeObjects[i];
        object->setVisible(origVisibilities[i]);
    }

    UNHIDE(m_editor->m_background, Background);
    UNHIDE(m_editor->m_groundLayer, GroundLayer);
    UNHIDE(m_editor->m_groundLayer2, GroundLayer2);
    UNHIDE(m_editor->m_middleground, Middleground);
    UNHIDE(m_editor->m_drawGridLayer, DrawGridLayer);

    Popup::draw();
}

void ExplodedLayerPopup::drawObjectLayers() {
    for (auto node : m_cachedGameplayNodes) {
        node->visit();
    }

    m_layerLabel->setString(fmt::format("{}", m_currentLayer).c_str());
    m_layerLabel->setVisible(true);
    m_layerLabel->visit();
    m_layerLabel->setVisible(false);
}

$on_mod(Loaded) {
    devtools::waitForDevTools([] {
        devtools::registerNode<ExplodedLayerPopup>([](ExplodedLayerPopup* popup) {
            devtools::property("layer offset", popup->m_layerOffset);
        });
    });
}
