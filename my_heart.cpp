// heart as graphics object
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>
#include <SFML/Graphics.hpp>

using namespace std;

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Heart Shape");

    // Создаём форму сердца
    sf::ConvexShape heart;
    heart.setPointCount(100);

    // Задаём форму сердца (по параметрическому уравнению)
    float scale = 15.0f;
    float center_x = 400;
    float center_y = 300;

    for (int i = 0; i < 100; ++i) {
        float t = i * 2 * M_PI / 100;
        float x = 16 * pow(sin(t), 3);
        float y = 13 * cos(t) - 5 * cos(2*t) - 2 * cos(3*t) - cos(4*t);

        heart.setPoint(i, sf::Vector2f(center_x + x * scale, center_y - y * scale));
    }

    heart.setFillColor(sf::Color::Red);

    // Основной цикл
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear(sf::Color::White);
        window.draw(heart);
        window.display();
    }

    return 0;
}

