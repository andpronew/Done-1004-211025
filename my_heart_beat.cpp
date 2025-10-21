#include <SFML/Graphics.hpp>
#include <cmath>

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Pulsating Heart");

    sf::ConvexShape heart;
    heart.setPointCount(100);

    float center_x = 400;
    float center_y = 300;

    sf::Clock clock;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Вычисляем текущее "время жизни" программы
        float time = clock.getElapsedTime().asSeconds();

        // Модифицируем масштаб — создаём эффект пульсации с помощью синуса
        float scale = 15.0f + std::sin(time * 3.0f) * 2.0f;

        for (int i = 0; i < 100; ++i) {
            float t = i * 2 * M_PI / 100;
            float x = 16 * pow(std::sin(t), 3);
            float y = 13 * std::cos(t) - 5 * std::cos(2*t) - 2 * std::cos(3*t) - std::cos(4*t);

            heart.setPoint(i, sf::Vector2f(center_x + x * scale, center_y - y * scale));
        }

        heart.setFillColor(sf::Color::Red);

        window.clear(sf::Color::White);
        window.draw(heart);
        window.display();
    }

    return 0;
}
