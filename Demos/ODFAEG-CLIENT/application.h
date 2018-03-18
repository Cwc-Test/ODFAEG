
#include <iostream>
#include <string>
#include <vector>
// *** END ***
#ifndef MY_APPLI
#define MY_APPLI
#include "odfaeg/Core/application.h"
#include "odfaeg/Graphics/convexShape.h"
#include "odfaeg/Graphics/rectangleShape.h"
#include "odfaeg/Graphics/circleShape.h"
#include "odfaeg/Graphics/tile.h"
#include "odfaeg/Core/world.h"
#include "odfaeg/Graphics/map.h"
#include "odfaeg/Graphics/2D/decor.h"
#include "odfaeg/Graphics/anim.h"
#include "odfaeg/Graphics/2D/ambientLight.h"
#include "odfaeg/Graphics/2D/ponctualLight.h"
#include "odfaeg/Graphics/2D/wall.h"
#include "odfaeg/Graphics/tGround.h"
#include "odfaeg/Core/actionMap.h"
#include "odfaeg/Graphics/entitiesUpdater.h"
#include "odfaeg/Graphics/animationUpdater.h"
#include "hero.h"
#include "monster.h"
#include "odfaeg/Network/network.h"
#include "odfaeg/Graphics/GUI/label.hpp"
#include "odfaeg/Graphics/GUI/button.hpp"
#include "odfaeg/Graphics/GUI/textArea.hpp"
#include "odfaeg/Graphics/GUI/passwordField.hpp"
#include "odfaeg/Graphics/GUI/progressBar.hpp"
#include <fstream>
namespace sorrok {
    class MyAppli : public odfaeg::core::Application,
                    public odfaeg::graphic::gui::ActionListener {
    private :
        const float speed = 0.2f;
        odfaeg::graphic::EntitiesUpdater *eu;
        odfaeg::graphic::AnimUpdater *au;
        bool running;
        odfaeg::graphic::g2d::Wall *w;
        Caracter* hero;
        Caracter* monster;
        sf::Keyboard::Key actualKey, previousKey;
        std::vector<odfaeg::graphic::Tile*> tiles;
        std::vector<odfaeg::graphic::Tile*> walls;
        odfaeg::graphic::Map* theMap;
        odfaeg::graphic::g2d::PonctualLight* light2;
        odfaeg::core::ResourceCache<> cache;
        sf::Time timeBtwnTwoReq = sf::seconds(1.f);
        sf::Int64 ping;
        bool received = false;
        static const unsigned int PATH_ERROR_MARGIN = 2;
        odfaeg::graphic::RenderWindow* wResuHero, *wIdentification;
        odfaeg::graphic::gui::Label* label, *labPseudo, *labMdp;
        odfaeg::graphic::gui::TextArea* taPseudo;
        odfaeg::graphic::gui::PasswordField* taPassword;
        odfaeg::graphic::gui::Button* button, *idButton, *invButton;
        odfaeg::graphic::gui::ProgressBar* hpBar, *xpBar;
        bool isClientAuthentified;
    public :
        enum Fonts {
            Serif
        };
        MyAppli(sf::VideoMode wm, std::string title);
        void keyHeldDown (sf::Keyboard::Key key);
        void leftMouseButtonPressed(sf::Vector2f mousePos);
        void rightMouseButtonPressed(sf::Vector2f mousePos);
        bool mouseInside (sf::Vector2f mousePos);
        void onMouseInside (sf::Vector2f mousePos);
        void onLoad();
        void onInit ();
        void onRender(odfaeg::graphic::RenderComponentManager *cm);
        void onDisplay(odfaeg::graphic::RenderWindow* window);
        void onUpdate (odfaeg::graphic::RenderWindow* window, sf::Event& event);
        void onExec ();
        void actionPerformed(odfaeg::graphic::gui::Button* item);
    };
}
#endif // MY_APPLI

