#pragma once
#include <undefined0.3d-api/include/3DAPI.hpp>
#include <alphalaneous.alphas-ui-pack/include/nodes/RenderNode.hpp>
#include "RenderNodeLier.hpp"

struct ShitPair {
    ThreeDeeAPI::ThirdDimensionObject* object;
    geode::Ref<alpha::ui::RenderNode> renderNode;
};

enum class LayersType {
    EditorLayers, VisualLayers
};

class ExplodedLayerPopup : public geode::Popup {
public:
    static ExplodedLayerPopup* get();
    static ExplodedLayerPopup* create();
    virtual bool init() override;

    virtual void onEnter() override;
    virtual void onClose(cocos2d::CCObject* sender) override;

    LevelEditorLayer* m_editor;
    EditorUI* m_editorUI;
    int m_oldEditorMode;
    float m_oldEditorToolbarHeight;
    std::vector<geode::Ref<GameObject>> m_oldSelectedObjects;

    geode::Ref<ThreeDeeAPI::ThirdDimensionNode> m_bg;
    ThreeDeeAPI::ThirdDimensionObject* m_bgCube;
    geode::Ref<RenderNodeLier> m_bgLier;
    geode::Ref<RenderNodeLier> m_groundLier;
    geode::Ref<alpha::ui::RenderNode> m_editorBGRenderNode;
    geode::Ref<alpha::ui::RenderNode> m_editorGroundRenderNode;

    geode::Ref<RenderNodeLier> m_lier;
    ThreeDeeAPI::ThirdDimensionNode* m_3dNode;
    std::map<int, ShitPair> m_layers;
    cocos2d::CCLabelBMFont* m_layerLabel;

    int m_currentLayer; // for sharing info between frames
    bool m_currentLayerHasObjects;
    std::vector<cocos2d::CCNode*> m_cachedGameplayNodes;
    LayersType m_currentLayersType;

    float m_layerOffset;
    float m_layerGap;

    virtual bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    virtual void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    virtual void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    virtual void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    virtual void scrollWheel(float y, float x) override;
    virtual void keyDown(cocos2d::enumKeyCodes key, double timestamp) override;
    virtual void keyUp(cocos2d::enumKeyCodes key, double timestamp) override;

    void updateLayersMap();

    virtual void update(float dt) override;
    virtual void draw() override;
    void drawObjectLayers();
    void drawBackground();
    void drawGround();
};
