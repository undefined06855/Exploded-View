#include "ExplodedLayerPopup.hpp"
#include <cue/RadioLogic.hpp>
#include <geode.devtools/include/API.hpp>
#include <ranges>

ExplodedLayerPopup* ExplodedLayerPopup::get() {
    return cocos2d::CCScene::get()->getChildByType<ExplodedLayerPopup>(0);
}

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
    m_editorUI = m_editor->m_editorUI;

    m_lier = RenderNodeLier::create([this] { this->drawObjectLayers(); });

    // TODO: fix whatever fuuucked up perspective we have rn like get an actually good one please thanks
    m_3dNode = ThreeDeeAPI::ThirdDimensionNode::create();
    m_3dNode->setID("3d-node");
    // m_3dNode->setCameraAim({ 84.f, 53.f, -199.f });
    // m_3dNode->setCameraPosition({ 242.f, 254.f, 57.f });
    // m_3dNode->setCameraFOV(70.f);
    m_3dNode->setCameraAim({ 112.f, 5.f, -199.f });
    m_3dNode->setCameraPosition({ 147.f, 53.f, 5.f });
    m_3dNode->setCameraFOV(120.f);
    m_3dNode->setFlipY(true);

    m_layerLabel = cocos2d::CCLabelBMFont::create("...", "bigFont.fnt");
    m_layerLabel->setID("current-layer-label");
    m_layerLabel->setPosition({ winSize.width / 2.f, winSize.height - 30.f });
    m_mainLayer->addChild(m_layerLabel);

    // TODO: make this be customisable or something?
    m_layerGap = 23.f;
    m_layerOffset = 5.f;

    // TODO: save and load this shit
    m_currentLayersType = LayersType::VisualLayers;

    m_mainLayer->addChildAtPosition(m_3dNode, geode::Anchor::Center);

    m_bg = ThreeDeeAPI::ThirdDimensionNode::create();
    m_bg->setID("bg-node");
    m_bg->setZOrder(-5);
    m_bg->setCameraPosition({ -.7f, 0.f, 0.f });
    m_bg->setCameraAim({ 10.f, 0.f, 0.f });
    m_bg->setCameraFOV(120.f);

    m_bgCube = ThreeDeeAPI::ThirdDimensionObject::create();
    m_bgCube->setID("bg-cube");
    auto ret = m_bgCube->loadObject("cube.obj"_spr);
    if (ret.isErr()) {
        geode::log::error("error loading: {}", ret.unwrapErr());
        return true;
    }

    m_bgCube->setScale(200.f); // ??? not sure why this is needed because of the rendertexture? it's weird
    m_bgCube->setRotationX(90.f);
    m_bgCube->setRotationZ(60.f);

    m_bg->addChild(m_bgCube);

    m_bgLier = RenderNodeLier::create([this] { this->drawBackground(); });
    m_editorBGRenderNode = alpha::ui::RenderNode::create(m_bgLier);
    m_editorBGRenderNode->initFBO();
    m_bgCube->setTexture(m_editorBGRenderNode->getTexture());

    // TODO: doesnt actually work :tada:
    auto bgGround = ThreeDeeAPI::ThirdDimensionObject::create();
    m_bgCube->setID("bg-ground");
    ret = bgGround->loadObject("plane.obj"_spr);
    if (ret.isErr()) {
        geode::log::error("error loading: {}", ret.unwrapErr());
        return true;
    }

    bgGround->setScale(700.f);
    bgGround->setRotationX(90.f);
    bgGround->setRotationZ(18.f);
    bgGround->setPositionY(70.f);
    m_bg->addChild(bgGround);

    m_groundLier = RenderNodeLier::create([this] { this->drawGround(); });
    m_editorGroundRenderNode = alpha::ui::RenderNode::create(m_groundLier);
    m_editorGroundRenderNode->initFBO();

    auto groundTexture = m_editorGroundRenderNode->getTexture();
    auto texParams = cocos2d::ccTexParams{ GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT };
    groundTexture->setTexParameters(&texParams);
    bgGround->setTexture(groundTexture);

    // probably the only time im going to actually use rendernode properly properly without a lier
    auto bgRenderNode = alpha::ui::RenderNode::create(m_bg, /* constrain */ true);
    bgRenderNode->setID("bg-render-node");
    bgRenderNode->setOpacity(0);
    bgRenderNode->runAction(cocos2d::CCFadeIn::create(1.f));

    m_mainLayer->addChildAtPosition(bgRenderNode, geode::Anchor::Center);

    cue::RadioLogic<LayersType> radio;
    auto editorLayers = radio.createToggler(LayersType::EditorLayers, .8f);
    auto visualLayers = radio.createToggler(LayersType::VisualLayers, .8f);

    radio.setCallback([this](LayersType which) {
        m_currentLayersType = which;
        this->updateLayersMap();
    });
    radio.select(m_currentLayersType);

    m_buttonMenu->addChildAtPosition(editorLayers, geode::Anchor::TopRight, { -132.f, -25.f });
    m_buttonMenu->addChildAtPosition(visualLayers, geode::Anchor::TopRight, { -132.f, -57.f });

    auto editorLayersLabel = cocos2d::CCLabelBMFont::create("Editor Layers", "bigFont.fnt");
    auto visualLayersLabel = cocos2d::CCLabelBMFont::create("Visual Layers", "bigFont.fnt");

    editorLayersLabel->setScale(.45f);
    visualLayersLabel->setScale(.45f);
    editorLayersLabel->setAnchorPoint({ 0.f, .5f });
    visualLayersLabel->setAnchorPoint({ 0.f, .5f });

    m_buttonMenu->addChildAtPosition(editorLayersLabel, geode::Anchor::TopRight, { -115.f, -25.f });
    m_buttonMenu->addChildAtPosition(visualLayersLabel, geode::Anchor::TopRight, { -115.f, -57.f });

    m_closeBtn->setLayoutOptions(
        geode::AnchorLayoutOptions::create()
            ->setOffset({ 25.f, -25.f })
            ->setAnchor(geode::Anchor::TopLeft)
    );
    m_buttonMenu->updateLayout();

    m_bgSprite->removeFromParent();

    this->setOpacity(0);
    this->scheduleUpdate();
    this->setMouseEnabled(true);

    return true;
}

void ExplodedLayerPopup::onEnter() {
    Popup::onEnter();

    m_oldEditorMode = m_editorUI->m_selectedMode;
    m_editorUI->m_selectedMode = 3;
    m_oldEditorToolbarHeight = m_editorUI->m_toolbarHeight;
    m_editorUI->m_toolbarHeight = 0.f;

    for (auto object : m_editorUI->m_selectedObjects->asExt<GameObject*>()) {
        m_oldSelectedObjects.push_back(object);
    }
    m_editorUI->deselectAll();
}

void ExplodedLayerPopup::onClose(cocos2d::CCObject* sender) {
    Popup::onClose(sender);
    m_editorUI->m_selectedMode = m_oldEditorMode;
    m_editorUI->m_toolbarHeight = m_oldEditorToolbarHeight;

    auto oldSelectedObjects = cocos2d::CCArray::createWithCapacity(m_oldSelectedObjects.size());
    for (auto object : m_oldSelectedObjects) oldSelectedObjects->addObject(object);
    m_editorUI->changeSelectedObjects(oldSelectedObjects, true);
}

bool ExplodedLayerPopup::ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    return m_editorUI->ccTouchBegan(touch, event);
}

void ExplodedLayerPopup::ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    m_editorUI->ccTouchMoved(touch, event);
}

void ExplodedLayerPopup::ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    m_editorUI->ccTouchEnded(touch, event);
}

void ExplodedLayerPopup::ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    m_editorUI->ccTouchCancelled(touch, event);
}

void ExplodedLayerPopup::scrollWheel(float y, float x) {
    m_layerOffset += y;
}

void ExplodedLayerPopup::keyDown(cocos2d::enumKeyCodes key, double timestamp) {
    if (key == cocos2d::enumKeyCodes::KEY_Escape) {
        Popup::keyDown(key, timestamp);
        return;
    }

    // TODO: fun for debugging
    if (key == cocos2d::enumKeyCodes::KEY_Enter) {
        m_editorUI->onPlaytest(m_editorUI->m_playtestBtn);
        return;
    }

    // m_editorUI->keyDown(key, timestamp);
}

void ExplodedLayerPopup::keyUp(cocos2d::enumKeyCodes key, double timestamp) {
    if (key == cocos2d::enumKeyCodes::KEY_Escape) {
        Popup::keyUp(key, timestamp);
        return;
    }

    // m_editorUI->keyUp(key, timestamp);
}

void ExplodedLayerPopup::updateLayersMap() {
    m_layers.clear();
    m_3dNode->removeAllChildren();

    std::unordered_set<int> layers;

    if (m_currentLayersType == LayersType::VisualLayers) {
        layers.insert((int)ZLayer::T4);
        layers.insert((int)ZLayer::T3);
        layers.insert((int)ZLayer::T2);
        layers.insert((int)ZLayer::T1);
        layers.insert((int)ZLayer::B1);
        layers.insert((int)ZLayer::B2);
        layers.insert((int)ZLayer::B3);
        layers.insert((int)ZLayer::B4);
        layers.insert((int)ZLayer::B5);
    } else {

        for (auto object : geode::cocos::CCArrayExt<GameObject*>(m_editor->m_objects)) {
            layers.insert(object->m_editorLayer);

            if (object->m_editorLayer2 != 0) {
                layers.insert(object->m_editorLayer2);
            }
        }
    }

    for (auto layer : layers) {
        auto renderNode = alpha::ui::RenderNode::create(m_lier);
        // do this here and not automatically because we need the texture later
        // else we get funny cc_2x2_white_square
        renderNode->initFBO();

        auto plane = ThreeDeeAPI::ThirdDimensionObject::create();
        plane->setTag(layer); // for devtools only

        auto ret = plane->loadObject("plane.obj"_spr);
        if (ret.isErr()) {
            geode::log::error("error loading: {}", ret.unwrapErr());
            return;
        }

        auto winSize = cocos2d::CCDirector::get()->getWinSize();
        plane->setScaleX(winSize.width);
        plane->setScaleY(winSize.height);

        plane->setTexture(renderNode->getTexture());

        m_3dNode->addChild(plane);

        m_layers[layer] = { plane, renderNode };
    }
}

void ExplodedLayerPopup::update(float dt) {
    float offset = 0.f;

    auto layerCallbackFn = [this, &offset](ShitPair& layer) {
        layer.object->setPositionZ(offset + m_layerOffset);
        layer.object->setZOrder(m_layers.size() - (-offset / m_layerGap));

        // -z is away from the camera
        offset -= m_layerGap;
    };

    if (m_currentLayersType == LayersType::VisualLayers) {
        // reverse iter for visual layers
        for (auto [_, layer] : m_layers | std::views::reverse) { layerCallbackFn(layer); }
    } else {
        for (auto [_, layer] : m_layers) { layerCallbackFn(layer); }
    }

    m_bgCube->setRotationX(m_bgCube->getRotationX() + dt*10.f);
    m_bgCube->setRotationY(m_bgCube->getRotationY() + dt*10.f);
    m_bgCube->setRotationZ(m_bgCube->getRotationZ() + dt*10.f);
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

    auto iter = std::views::reverse(m_layers);

    for (auto& [ layer, data ] : m_layers) {
        // remember -z is out of the camera and +z is towards the camera
        data.object->setVisible(false);
        if (data.object->getPositionZ() > 0.f) continue; // planes too close
        if (data.object->getPositionZ() < -m_layerGap * 14.f) continue; // planes too far
        data.object->setVisible(true);

        m_currentLayerHasObjects = false;
        for (int i = 0; i < m_editor->m_activeObjectsCount; i++) {
            auto object = m_editor->m_activeObjects[i];

            bool visible;
            if (m_currentLayersType == LayersType::VisualLayers) {
                visible = (int)object->getObjectZLayer() == layer;
            } else {
                visible = object->m_editorLayer == layer || (object->m_editorLayer2 != 0 && object->m_editorLayer2 == layer);
            }

            if (visible) m_currentLayerHasObjects = true;

            object->setVisible(visible);
        }

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

    m_editorBGRenderNode->render();

    Popup::draw();
}

void ExplodedLayerPopup::drawObjectLayers() {
    if (m_currentLayerHasObjects) {
        for (auto node : m_cachedGameplayNodes) {
            node->visit();
        }
    }

    if (m_currentLayersType == LayersType::VisualLayers) {
        static std::unordered_map<int, std::string> layerLabelLookup = {
            { (int)ZLayer::B5, "B5" },
            { (int)ZLayer::B4, "B4" },
            { (int)ZLayer::B3, "B3" },
            { (int)ZLayer::B2, "B2" },
            { (int)ZLayer::B1, "B1" },
            { (int)ZLayer::T1, "T1" },
            { (int)ZLayer::T2, "T2" },
            { (int)ZLayer::T3, "T3" },
            { (int)ZLayer::T4, "T4" },
        };

        m_layerLabel->setString(layerLabelLookup[m_currentLayer].c_str());
    } else {
        m_layerLabel->setString(fmt::to_string(m_currentLayer).c_str());
    }
    m_layerLabel->setVisible(true);
    m_layerLabel->visit();
    m_layerLabel->setVisible(false);
}

void ExplodedLayerPopup::drawBackground() {
    auto origPos = m_editor->m_background->getPosition();
    m_editor->m_background->setPosition({ 0.f, 0.f });
    m_editor->m_background->visit();
    m_editor->m_background->setPosition(origPos);
}

void ExplodedLayerPopup::drawGround() {
    auto origVisibility = m_editor->m_groundLayer->isVisible();
    m_editor->m_groundLayer->setVisible(true);
    for (auto child : m_editor->m_groundLayer->getChildrenExt()) {
        if (!geode::cast::exact_cast<cocos2d::CCSprite*>(child)) continue;
        child->setVisible(false);
    }

    m_editor->m_groundLayer->visit();

    for (auto child : m_editor->m_groundLayer->getChildrenExt()) {
        if (!geode::cast::exact_cast<cocos2d::CCSprite*>(child)) continue;
        child->setVisible(true);
    }

    m_editor->m_groundLayer->setVisible(false);
}

$on_mod(Loaded) {
    devtools::waitForDevTools([] {
        devtools::registerNode<ExplodedLayerPopup>([](ExplodedLayerPopup* popup) {
            devtools::property("layer gap", popup->m_layerGap);
            devtools::property("layer offset", popup->m_layerOffset);
        });
    });
}
