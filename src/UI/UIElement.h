#pragma once
namespace DDD {
class Renderer;
class UIElement {
public:
    UIElement() = default;
    virtual ~UIElement() = default;
    virtual void Update(float deltaTime) {}
    virtual void Render(Renderer& renderer) {}
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
protected:
    bool m_visible = true;
};
} // namespace DDD
