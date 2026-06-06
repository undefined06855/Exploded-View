#pragma once

// pants on fire
class RenderNodeLier : public cocos2d::CCNode {
public:
    static RenderNodeLier* create(geode::Function<void()> callback);
    virtual bool init(geode::Function<void()> callback);

    geode::Function<void()> m_callback;

    virtual void visit() override;
};
