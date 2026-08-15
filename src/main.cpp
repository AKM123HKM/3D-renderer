#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <cmath>

constexpr int WIDTH = 800;
constexpr int HEIGHT = 800;
constexpr float VIEWPORT_W = 1;
constexpr float VIEWPORT_H = 1;

sf::Color multiplyColorWithIntensity(sf::Color color,float intensity){
    int r = static_cast<int>(color.r) * intensity;
    int g = static_cast<int>(color.g) * intensity;
    int b = static_cast<int>(color.b) * intensity;
    return sf::Color(r,g,b);
}

float dotProduct(sf::Vector3f v1, sf::Vector3f v2){
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

float magnitude(sf::Vector3f v){
    return sqrt(dotProduct(v,v));
}

sf::Vector3f normalize(sf::Vector3f v){
    float mag = magnitude(v);
    if(!(mag == 0)){
        return (1/mag) * v;
    }
    return sf::Vector3f(-1,-1,-1);
}

struct Sphere{
    sf::Vector3f position = sf::Vector3f(0,0,0);
    sf::Color color = sf::Color(0,0,0);;
    int radius = 0;

    Sphere(sf::Vector3f Apos,sf::Color Acolor,int Aradius): position(Apos),color(Acolor),radius(Aradius){}

    Sphere() = default;

    bool operator==(const Sphere&) const = default;

    bool isNull() const{
        return *this == Sphere{};
    }
};

enum LightType{
    Ambient,
    Point,
    Directional
};

struct Light{
    LightType type = Ambient;
    float intensity = 0;
    sf::Vector3f dirOrPos = sf::Vector3f(0,0,0);

    Light(LightType Atype,float Aintensity,sf::Vector3f AdirOrPos = sf::Vector3f(0,0,0)):type(Atype),intensity(Aintensity),dirOrPos(AdirOrPos){}

    Light() = default;
};

sf::Vertex getPixel(int x,int y,sf::Color color){
    int Sx = WIDTH/2 + x;
    int Sy = HEIGHT/2 - y;
    return sf::Vertex (sf::Vector2f(Sx,Sy),color);
}

sf::Vector3f canvasToViewPort(int x,int y){
    float Vx = x * (VIEWPORT_W/WIDTH);
    float Vy = y * (VIEWPORT_H/HEIGHT);
    return sf::Vector3f(Vx,Vy,1);
}

float computeLighthing(sf::Vector3f P,sf::Vector3f N,std::vector<Light> lights){
    float i = 0;
    for(auto light:lights){
        if(light.type == Ambient){
            i += light.intensity;
        }
        else{
            sf::Vector3f L;
            if(light.type == Point){
                L = light.dirOrPos - P;
            }
            else{
                L = light.dirOrPos;
            }
            if(!(dotProduct(L,N) < 0 || magnitude(L) == 0 || magnitude(N) == 0)){
                i += light.intensity * dotProduct(L,N)/(magnitude(L) * magnitude(N));
            }
        }
    }
    return i;
}

std::pair<float,float> intersectRaySphere(sf::Vector3f origin,sf::Vector3f ViewPortCoords,Sphere sphere){
    sf::Vector3f CO = origin - sphere.position;
    float a = dotProduct(ViewPortCoords,ViewPortCoords);
    float b = 2 * dotProduct(CO,ViewPortCoords);
    float c = dotProduct(CO,CO) - pow(sphere.radius,2);

    float discriminant = pow(b,2) - 4*a*c;
    if (discriminant < 0){
        return {INFINITY,INFINITY};
    }

    float t1 = (-b + sqrt(discriminant))/(2*a);
    float t2 = (-b - sqrt(discriminant))/(2*a);
    return {t1,t2};
}

sf::Color traceRay(sf::Vector3f origin,sf::Vector3f ViewPortCoords,float t_min,float t_max,std::vector<Sphere> spheres,std::vector<Light> lights){
    float closest_t = INFINITY;
    Sphere closest_sphere;

    for (auto sphere:spheres){
        auto [t1,t2] = intersectRaySphere(origin,ViewPortCoords,sphere);  
        
        if(t1 >= t_min & t1 <= t_max & t1 < closest_t){
            closest_t = t1;
            closest_sphere = sphere;
        }
        if(t2 >= t_min & t2 <= t_max & t2 < closest_t){
            closest_t = t2;
            closest_sphere = sphere;
        }
    }
    if(!(closest_sphere.isNull())){
        sf::Vector3f P = origin + closest_t*(ViewPortCoords);
        sf::Vector3f N = P - closest_sphere.position;
        return multiplyColorWithIntensity(closest_sphere.color,computeLighthing(P,normalize(N),lights));
    }
    return sf::Color::White;
}

int main() {
    sf::RenderWindow window(sf::VideoMode({WIDTH,HEIGHT}), "SFML 3 Test");
    sf::Clock clock;
    const sf::Font font("../assets/PoetsenOne-Regular.ttf");
    sf::Text fps(font);
    fps.setCharacterSize(30);
    fps.setFillColor(sf::Color::Black);
    fps.setPosition(sf::Vector2f(0,0));
    
    // Camera position
    sf::Vector3f origin = sf::Vector3f(0,0,0);
    
    // Objects in the scene
    std::vector<Sphere> spheres;
    spheres.emplace_back(sf::Vector3f(0,-1,3),sf::Color::Red,1);
    spheres.emplace_back(sf::Vector3f(2,0,4),sf::Color::Green,1);
    spheres.emplace_back(sf::Vector3f(-2,0,4),sf::Color::Blue,1);
    spheres.emplace_back(sf::Vector3f(0,-5001,0),sf::Color::Yellow,5000);

    //Lights in the scene
    std::vector<Light> lights;
    lights.emplace_back(Point,0.6,sf::Vector3f(2,1,0));
    lights.emplace_back(Ambient,0.2);
    lights.emplace_back(Directional,0.2,sf::Vector3f(1,4,4));

    while (window.isOpen()) {
        fps.setString(std::to_string(1/clock.getElapsedTime().asSeconds()));
        clock.restart();
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }
        window.clear(sf::Color::Black);
        sf::VertexArray frame(sf::PrimitiveType::Points);
        frame.resize(WIDTH*HEIGHT);
        for(int x = -WIDTH/2; x <= WIDTH/2; x++){
            for(int y = -HEIGHT/2; y <= HEIGHT/2; y++){
                sf::Vector3f D = canvasToViewPort(x,y);
                sf::Color color = traceRay(origin,D,1,INFINITY,spheres,lights);
                frame.append(getPixel(x,y,color));
            }
        }

        window.draw(frame);
        window.draw(fps);
        window.display();
    }
}
