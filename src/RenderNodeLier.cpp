#include "RenderNodeLier.hpp"

RenderNodeLier* RenderNodeLier::create(geode::Function<void()> callback) {
    auto ret = new RenderNodeLier;
    if (ret->init(std::move(callback))) {
        ret->autorelease();
        return ret;
    }

    delete ret;
    return nullptr;
}

bool RenderNodeLier::init(geode::Function<void()> callback) {
    if (!CCNode::init()) return false;

    m_callback = std::move(callback);

    return true;
}

void RenderNodeLier::visit() {
    m_callback();
}
