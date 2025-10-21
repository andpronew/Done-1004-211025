// circles as graphics objects 
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>
#include <SFML/Graphics.hpp>

using namespace std;

class Circle {
public:
    Circle(float x, float y, float r, sf::Color color) {
        circle_.setRadius(r);
        circle_.setFillColor(color);
        circle_.setPosition(x - r, y - r);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(circle_);
    }

private:
    sf::CircleShape circle_;
};

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "SFML Circles");

    	Circle c1(150, 100, 50, sf::Color::Blue);
	Circle c2(400, 200, 70, sf::Color::Red);
	Circle c3(650, 300, 40, sf::Color::Green);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear(sf::Color::White);
        c1.draw(window);
        c2.draw(window);
        c3.draw(window);
        window.display();
    }

    return 0;
}

