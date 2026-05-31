#pragma once
#include <undefined0.3d-api/include/3DAPI.hpp>
#include <alphalaneous.alphas-ui-pack/include/nodes/RenderNode.hpp>
#include "RenderNodeLier.hpp"

struct ShitPair {
    ThreeDeeAPI::ThirdDimensionObject* object;
    geode::Ref<alpha::ui::RenderNode> renderNode;
};

class ExplodedLayerPopup : public geode::Popup {
public:
    static ExplodedLayerPopup* create();
    virtual bool init() override;

    virtual void onClose(cocos2d::CCObject* sender) override;

    LevelEditorLayer* m_editor;
    int m_oldEditorMode;

    geode::Ref<RenderNodeLier> m_lier;
    ThreeDeeAPI::ThirdDimensionNode* m_3dNode;
    std::unordered_map<int, ShitPair> m_layers;
    cocos2d::CCLabelBMFont* m_layerLabel;

    int m_currentLayer;
    std::vector<cocos2d::CCNode*> m_cachedGameplayNodes;

    float m_layerOffset;
    float m_layerGap;

    virtual bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    virtual void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    virtual void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    virtual void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    virtual void scrollWheel(float y, float x) override;

    virtual void update(float dt) override;
    virtual void draw() override;
    void drawObjectLayers();
};
