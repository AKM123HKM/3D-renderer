#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <cmath>

constexpr int WIDTH = 800;
constexpr int HEIGHT = 800;
constexpr float VIEWPORT_W = 1;
constexpr float VIEWPORT_H = 1;

sf::Color multiplyColorWithIntensity(sf::Color color,float intensity){
    int r = std::min(static_cast<int>(color.r) * intensity,255.f);
    int g = std::min(static_cast<int>(color.g) * intensity,255.f);
    int b = std::min(static_cast<int>(color.b) * intensity,255.f);
    return sf::Color(r,g,b);
}

sf::Color addColors(sf::Color c1, sf::Color c2){
    int r = std::min(static_cast<int>(c1.r) + static_cast<int>(c2.r),255);
    int g = std::min(static_cast<int>(c1.g) + static_cast<int>(c2.g),255);
    int b = std::min(static_cast<int>(c1.b) + static_cast<int>(c2.b),255);
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

sf::Vector3f reflect(sf::Vector3f v, sf::Vector3f n){
    return 2.f*normalize(n)*dotProduct(normalize(n),v) - v;
}

struct Sphere{
    sf::Vector3f position = sf::Vector3f(0,0,0);
    sf::Color color = sf::Color(0,0,0);;
    int radius = 0;
    int specular = -1;
    float reflectivness = 0.f;

    Sphere(sf::Vector3f Apos,sf::Color Acolor,int Aradius,int Aspecular,float Areflectivness): position(Apos),color(Acolor),radius(Aradius),specular(Aspecular),reflectivness(Areflectivness){}

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

std::pair<Sphere,float> closestIntersection(sf::Vector3f origin, sf::Vector3f viewPortCoords,float t_min,float t_max,std::vector<Sphere> spheres){
    float closest_t = INFINITY;
    Sphere closest_sphere;

    for (auto sphere:spheres){
        auto [t1,t2] = intersectRaySphere(origin,viewPortCoords,sphere);  
        
        if(t1 >= t_min && t1 <= t_max && t1 < closest_t){
            closest_t = t1;
            closest_sphere = sphere;
        }
        if(t2 >= t_min && t2 <= t_max && t2 < closest_t){
            closest_t = t2;
            closest_sphere = sphere;
        }
    }
    
    return {closest_sphere,closest_t};
}

float computeLighthing(sf::Vector3f P,sf::Vector3f N,sf::Vector3f origin,int s,std::vector<Light> lights,std::vector<Sphere> spheres){
    float i = 0;
    for(auto light:lights){
        if(light.type == Ambient){
            i += light.intensity;
        }
        else{
            sf::Vector3f L;
            float t_max;
            if(light.type == Point){
                L = light.dirOrPos - P;
                t_max = 1;
            }
            else{
                L = light.dirOrPos;
                t_max = INFINITY;
            }

            auto [shadow_sphere,shadow_t] = closestIntersection(P,L,0.001,t_max,spheres);
            if(!(shadow_sphere.isNull())){
                continue;
            }

            if(!(dotProduct(L,N) < 0 || magnitude(L) == 0 || magnitude(N) == 0)){
                i += light.intensity * dotProduct(L,N)/(magnitude(L) * magnitude(N));
            }

            if(!(s == -1)){
                sf::Vector3f R = reflect(L,N);
                sf::Vector3f V = origin - P;
                if(!(dotProduct(R,V) < 0 || magnitude(R) == 0 || magnitude(V) == 0)){
                    i += light.intensity * pow(dotProduct(R,V)/(magnitude(R) * magnitude(V)),100);
                }
            }
        }
    }
    return i;
}

sf::Color traceRay(sf::Vector3f origin,sf::Vector3f ViewPortCoords,float t_min,float t_max,std::vector<Sphere> spheres,std::vector<Light> lights,int recursion_depth){
    auto [closest_sphere,closest_t] = closestIntersection(origin,ViewPortCoords,t_min,t_max,spheres);
    if(!(closest_sphere.isNull())){
        sf::Vector3f P = origin + closest_t*(ViewPortCoords);
        sf::Vector3f N = P - closest_sphere.position;
        sf::Color local_color = multiplyColorWithIntensity(closest_sphere.color,computeLighthing(P,normalize(N),origin,closest_sphere.specular,lights,spheres));
        if(closest_sphere.reflectivness){
            sf::Vector3f R = reflect(-1.f*ViewPortCoords,N);
            if(recursion_depth >= 0 && closest_sphere.reflectivness >= 0){
                sf::Color reflected_color = traceRay(ViewPortCoords,R,0.001,INFINITY,spheres,lights,recursion_depth - 1);
                return addColors(multiplyColorWithIntensity(local_color,(1 - closest_sphere.reflectivness)),multiplyColorWithIntensity(reflected_color,closest_sphere.reflectivness));
            }
        }
        return local_color;
    }
    return sf::Color::Black;
}

int main() {
    sf::RenderWindow window(sf::VideoMode({WIDTH,HEIGHT}), "Raytracer");
    sf::Clock clock;
    const sf::Font font("assets/PoetsenOne-Regular.ttf");
    sf::Text fps(font);
    fps.setCharacterSize(30);
    fps.setFillColor(sf::Color::White);
    fps.setPosition(sf::Vector2f(0,0));
    
    // Camera position
    sf::Vector3f origin = sf::Vector3f(0,0,0);
    
    // Objects in the scene
    std::vector<Sphere> spheres;
    spheres.emplace_back(sf::Vector3f(0,-1,3),sf::Color::Red,1,500,0.2);
    spheres.emplace_back(sf::Vector3f(2,0,4),sf::Color::Green,1,500,0.3);
    spheres.emplace_back(sf::Vector3f(-2,0,4),sf::Color::Blue,1,10,0.4);
    spheres.emplace_back(sf::Vector3f(0,-5001,0),sf::Color::Yellow,5000,1000,0.5);
    // spheres.emplace_back(sf::Vector3f(0,2,4),sf::Color::Green,1,500);
    // spheres.emplace_back(sf::Vector3f(0,-0.5,4),sf::Color::Blue,1,500);

    //Lights in the scene
    std::vector<Light> lights;
    lights.emplace_back(Point,0.6,sf::Vector3f(5,0,-4));
    lights.emplace_back(Ambient,0.2);
    lights.emplace_back(Directional,0.4,sf::Vector3f(1,4,4));
    // lights.emplace_back(Directional,0.6,sf::Vector3f(0,1,0));

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
        for(int x = -WIDTH/2; x <= WIDTH/2; x ++){
            for(int y = -HEIGHT/2; y <= HEIGHT/2; y ++){
                sf::Vector3f D = canvasToViewPort(x,y);
                sf::Color color = traceRay(origin,D,1,INFINITY,spheres,lights,2);
                frame.append(getPixel(x,y,color));
            }
        }

        window.draw(frame);
        window.draw(fps);
        window.display();
    }
}
