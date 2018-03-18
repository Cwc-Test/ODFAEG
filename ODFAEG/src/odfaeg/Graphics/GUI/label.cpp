#include "../../../../include/odfaeg/Graphics/GUI/label.hpp"
namespace odfaeg {
    namespace graphic {
        namespace gui {
            Label::Label (RenderWindow& window, math::Vec3f position, math::Vec3f size, const Font* font, std::string t) :
            LightComponent (window, position, size, size * 0.5f) {
                text.setFont(*font);
                text.setString(t);
                text.setSize(size);
                unsigned int characterSize = size.y;
                text.setCharacterSize(characterSize);
                rect = RectangleShape (size);
                background = sf::Color::Black;
                rect.setFillColor(background);
            }
            void Label::setForegroundColor(sf::Color color) {
                text.setColor(color);
            }
            sf::Color Label::getForegroundColor() {
                return text.getColor();
            }
            void Label::setBackgroundColor(sf::Color color) {
                this->background = color;
            }
            sf::Color Label::getBackgroundColor() {
                return background;
            }
            void Label::clear() {
                if (background != rect.getFillColor()) {
                    rect.setFillColor(background);
                }
            }
            void Label::onDraw(RenderTarget& target, RenderStates states) {
                text.setPosition(getPosition());
                rect.setPosition(getPosition());
                text.setSize(getSize());
                rect.setSize(getSize());
                target.draw(rect);
                target.draw(text);
            }
            void Label::setText(std::string t) {
                text.setString(t);
            }
            std::string Label::getText() {
                return text.getString();
            }
            bool Label::isMouseInside() {
                physic::BoundingBox bb(getPosition().x, getPosition().y, 0, getSize().x, getSize().y, 0);
                return bb.isPointInside(mousePos);
            }
            void Label::onUpdate(RenderWindow* window, sf::Event& event) {
                LightComponent::onUpdate(window, event);
                if (&getWindow() == window) {
                    if (event.type == sf::Event::MouseButtonPressed) {
                        mousePos = math::Vec3f(event.mouseButton.x, event.mouseButton.y, 0);
                    }
                }
            }
            void Label::onEventPushed (sf::Event event, RenderWindow& window) {
                if (&getWindow() == &window)
                    getListener().pushEvent(event);
            }
            void Label::checkSubWindowEvents() {
            }
        }
    }
}
