#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <cmath>

constexpr int WIDTH = 800;
constexpr int HEIGHT = 800;
constexpr int VIEWPORT_W = 1;
constexpr int VIEWPORT_H = 1;

std::pair<int,int> getOriginAtCenterCoords(int x, int y){
    return {WIDTH/2 + x, HEIGHT/2 - y};
}

sf::Vertex getPixel(int x,int y,sf::Color color){
    auto [Sx,Sy] = getOriginAtCenterCoords(x,y);
    return sf::Vertex (sf::Vector2f(Sx,Sy),color);
}

void swap(sf::Vector2f& v1, sf::Vector2f& v2){
    sf::Vector2f a = v1;
    v1 = v2;
    v2 = a;
}

std::vector<float> interpolate(float d1, float d2, float i1, float i2){
    std::vector<float> values;
    // values.reserve(static_cast<int>(i2 - i1) + 1);
    values.push_back(d1);
    if(i1 == i2){
        return values;
    }
    float m = (d2-d1)/(i2-i1);
    float d = d1;
    for(int i = i1; i <= i2; i++){
        d += m;
        values.push_back(d);
    }
    return values;
}

sf::VertexArray getLine(sf::Vector2f p1,sf::Vector2f p2, sf::Color color){
    sf::VertexArray line(sf::PrimitiveType::Points);
    if(abs(p1.x - p2.x) > abs(p1.y - p2.y)){
        if(p1.x > p2.x){
            swap(p1,p2);
        }
        auto values = interpolate(p1.y,p2.y,p1.x,p2.x);
        for(int x = p1.x; x <= p2.x; x++){
            line.append(getPixel(x,values[x-p1.x],color));
        }
    }

    else{
        if(p1.y > p2.y){
            swap(p1,p2);
        }
        auto values = interpolate(p1.x,p2.x,p1.y,p2.y);
        for(int y = p1.y; y <= p2.y; y++){
            line.append(getPixel(values[y-p1.y],y,color));
        }
    }
    return line;
};

std::vector<sf::VertexArray> getTriangle(sf::Vector2f l0, sf::Vector2f l1, sf::Vector2f l2, sf::Color color){
    if(l1.y < l0.y){
        swap(l0,l1);
    }
    if(l2.y < l0.y){
        swap(l0,l2);
    }
    if(l2.y < l1.y){
        swap(l1,l2);
    }
    std::vector<sf::VertexArray> lines;
    lines.reserve(abs(l2.y - l0.y) + 1);

    auto x_02 = interpolate(l0.x,l2.x,l0.y,l2.y);
    auto x_01 = interpolate(l0.x,l1.x,l0.y,l1.y);
    auto x_12 = interpolate(l1.x,l2.x,l1.y,l2.y);
    
    x_01.pop_back();
    for(float l:x_12){
        x_01.push_back(l);
    }
    for(int y = l0.y; y <= l2.y; y++){
        lines.push_back(getLine(sf::Vector2f(x_02[y -l0.y],y),sf::Vector2f(x_01[y - l0.y],y),color));
    }

    return lines;
}

sf::Vector3f canvasToViewPort(int x,int y){
    float Vx = x * (VIEWPORT_W/WIDTH);
    float Vy = y * (VIEWPORT_H/HEIGHT);
    return sf::Vector3f(Vx,Vy,1);
}

int main() {
    sf::RenderWindow window(sf::VideoMode({WIDTH,HEIGHT}), "Rasterizer");
    sf::Clock clock;
    const sf::Font font("../assets/PoetsenOne-Regular.ttf");
    sf::Text fps(font);
    fps.setCharacterSize(30);
    fps.setFillColor(sf::Color::White);
    fps.setPosition(sf::Vector2f(0,0));
    auto l1 = getLine(sf::Vector2f(-200, -250), sf::Vector2f(200, 50), sf::Color::White);
    auto l2 = getLine(sf::Vector2f(200, 50), sf::Vector2f(20, 250), sf::Color::White);
    auto l3 = getLine(sf::Vector2f(20, 250), sf::Vector2f(-200, -250), sf::Color::White);

    auto triangle = getTriangle(sf::Vector2f(-200, -250),sf::Vector2f(200, 50),sf::Vector2f(20, 250),sf::Color::Red);

    // auto l1 = getLine(sf::Vector2f(-150, -100), sf::Vector2f(150, -100), sf::Color::Red);
    // auto l2 = getLine(sf::Vector2f(150, -100), sf::Vector2f(0, 160), sf::Color::Red);
    // auto l3 = getLine(sf::Vector2f(0, 160), sf::Vector2f(-150, -100), sf::Color::Red);

    while (window.isOpen()) {
        fps.setString(std::to_string(1/clock.getElapsedTime().asSeconds()));
        clock.restart();
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }
        window.clear();
        window.draw(fps);
        window.draw(l1);
        window.draw(l2);
        window.draw(l3);
        for(auto& line:triangle){
            window.draw(line);
        }
        window.display();
    }
}
