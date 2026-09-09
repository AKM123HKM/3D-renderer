#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <cmath>
#include <array>

constexpr int WIDTH = 800;
constexpr int HEIGHT = 800;
constexpr int VIEWPORT_W = 4;
constexpr int VIEWPORT_H = 4;
constexpr float pi = 3.14159;
using Matrix = std::array<std::array<float, 4>, 4>;
using Object = std::vector<std::vector<sf::VertexArray>>;
constexpr Matrix IDENTITY_MATRIX{{
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f}
}};


struct Vector4f{
    float x;
    float y;
    float z;
    float w;
};

Vector4f operator*(const Matrix& mat, const Vector4f& vec) {
    Vector4f result;
    
    result.x = mat[0][0] * vec.x + mat[0][1] * vec.y + mat[0][2] * vec.z + mat[0][3] * vec.w;
    
    result.y = mat[1][0] * vec.x + mat[1][1] * vec.y + mat[1][2] * vec.z + mat[1][3] * vec.w;
    
    result.z = mat[2][0] * vec.x + mat[2][1] * vec.y + mat[2][2] * vec.z + mat[2][3] * vec.w;
    
    result.w = mat[3][0] * vec.x + mat[3][1] * vec.y + mat[3][2] * vec.z + mat[3][3] * vec.w;
    
    return result;
}

Matrix operator*(const Matrix& lhs, const Matrix& rhs) {
    Matrix result = IDENTITY_MATRIX;

    for (int col = 0; col < 4; ++col) {
        Vector4f rhsCol{ rhs[0][col], rhs[1][col], rhs[2][col], rhs[3][col] };
        Vector4f resCol = lhs * rhsCol;

        result[0][col] = resCol.x;
        result[1][col] = resCol.y;
        result[2][col] = resCol.z;
        result[3][col] = resCol.w;
    }

    return result;
}

Matrix operator*(const Matrix& mat, float scalar){
    Matrix result = mat;
    for(int col = 0; col < 4; col++){
        for(int row = 0; row < 4; row++){
            result[row][col] *= scalar;
        }
    }

    return result;
}

Vector4f get4DVector(sf::Vector3f v){
    return Vector4f(v.x,v.y,v.z,1);
}

sf::Vector3f get3DVector(Vector4f v){
    return sf::Vector3f(v.x/v.w, v.y/v.w, v.z/v.w);
}

Matrix transposeMatrix(const Matrix& mat){
    Matrix transposedMatrix = IDENTITY_MATRIX;
    for (int col = 0; col < 4; col++){
        for (int row = 0; row < 4; row++){
            transposedMatrix[col][row] = mat[row][col];
        }
    }

    return transposedMatrix;
}

Matrix getTranslationMatrix(sf::Vector3f translation){
    Matrix translationMatrix = IDENTITY_MATRIX;
    translationMatrix[0][3] = translation.x;
    translationMatrix[1][3] = translation.y;
    translationMatrix[2][3] = translation.z;

    return translationMatrix;
}

Matrix getInverseTranslationMatrix(sf::Vector3f translation){
    Matrix translationMatrix = IDENTITY_MATRIX;
    translationMatrix[0][3] = -translation.x;
    translationMatrix[1][3] = -translation.y;
    translationMatrix[2][3] = -translation.z;

    return translationMatrix;
}

Matrix getRotationMatrix(std::array<float,3> angles){
    // rotation matrix along Y axis
    float angle_in_radians = angles[1] * (pi/180.f);
    float cos = std::cos(angle_in_radians);
    float sin = std::sin(angle_in_radians);
    Matrix rotationMatrixY = IDENTITY_MATRIX;
    rotationMatrixY[0][0] = cos;
    rotationMatrixY[0][2] = sin;
    rotationMatrixY[2][0] = -sin;
    rotationMatrixY[2][2] = cos;

    // rotation matrix along z axis
    angle_in_radians = angles[2] * (pi/180.f);
    cos = std::cos(angle_in_radians);
    sin = std::sin(angle_in_radians);
    Matrix rotationMatrixZ = IDENTITY_MATRIX;
    rotationMatrixZ[0][0] = cos;
    rotationMatrixZ[0][1] = -sin;
    rotationMatrixZ[1][0] = sin;
    rotationMatrixZ[1][1] = cos;

    // rotation matrix along x axis
    angle_in_radians = angles[0] * (pi/180.f);
    cos = std::cos(angle_in_radians);
    sin = std::sin(angle_in_radians);
    Matrix rotationMatrixX = IDENTITY_MATRIX;
    rotationMatrixX[1][1] = cos;
    rotationMatrixX[1][2] = -sin;
    rotationMatrixX[2][1] = sin;
    rotationMatrixX[2][2] = cos;

    return rotationMatrixZ * rotationMatrixY * rotationMatrixX;
}

Matrix getScalingMatrix(float scalar){
    Matrix result = IDENTITY_MATRIX*scalar;
    result[3][3] = 1;
    return result;
}


struct Triangle{
    int v1;
    int v2;
    int v3;
    sf::Color color;

    Triangle():v1(0),v2(1),v3(2),color(sf::Color::Black){}
    Triangle(int aV1,int aV2,int aV3,sf::Color aColor):v1(aV1),v2(aV2),v3(aV3),color(aColor){}
};

struct Model{
    std::vector<sf::Vector3f> vertices;
    std::vector<Triangle> triangles;
};

struct Instance{
    Model* model_ptr;
    sf::Vector3f translation;
    std::array<float,3> angles;
    float scale;
};

sf::Vector3f canvasToViewPort(int x,int y){
    float Vx = x * (VIEWPORT_W/WIDTH);
    float Vy = y * (VIEWPORT_H/HEIGHT);
    return sf::Vector3f(Vx,Vy,1);
}

sf::Vertex getPixel(int x,int y,sf::Color color){
    float Sx = WIDTH/2 + x;
    float Sy = HEIGHT/2 - y;
    return sf::Vertex (sf::Vector2f(Sx,Sy),color);
}

void translateVertexes(std::vector<sf::Vector3f>& vertexes, sf::Vector3f translation){
    for(auto& vertex:vertexes){
        vertex += translation;
    }
}

template <typename T>
void swap(T& v1, T& v2){
    T a = v1;
    v1 = v2;
    v2 = a;
}

void printVector(sf::Vector2f vector){
    std::cout << "x: " << vector.x << ", y: " << vector.y << std::endl;
}

sf::Vector2f projectVector(sf::Vector3f vector,float viewport_z){
    sf::Vector2f viewport_coords = sf::Vector2f((vector.x * viewport_z)/vector.z, (vector.y * viewport_z)/vector.z);
    sf::Vector2f screen_coords = sf::Vector2f((viewport_coords.x / VIEWPORT_W) * WIDTH, (viewport_coords.y / VIEWPORT_H) * HEIGHT);

    return screen_coords;
}

sf::Color multiplyColorWithIntensity(sf::Color color,float intensity){
    int r = std::min(static_cast<int>(color.r) * intensity,255.f);
    int g = std::min(static_cast<int>(color.g) * intensity,255.f);
    int b = std::min(static_cast<int>(color.b) * intensity,255.f);
    return sf::Color(r,g,b);
}

std::vector<float> interpolate(float d1, float d2, float i1, float i2){
    std::vector<float> values;
    values.reserve(static_cast<int>(i2 - i1) + 1);
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

sf::VertexArray getLine(sf::Vector2f p1,sf::Vector2f p2, sf::Color color,float h1, float h2){
    sf::VertexArray line(sf::PrimitiveType::Points);
    if(abs(p1.x - p2.x) > abs(p1.y - p2.y)){
        if(p1.x > p2.x){
            swap(p1,p2);
            swap(h1,h2);
        }
        auto h_values = interpolate(h1,h2,p1.x,p2.x);
        auto values = interpolate(p1.y,p2.y,p1.x,p2.x);
        for(int x = p1.x; x <= p2.x; x++){
            line.append(getPixel(x,values[x-p1.x],multiplyColorWithIntensity(color,h_values[x-p1.x])));
        }
    }

    else{
        if(p1.y > p2.y){
            swap(p1,p2);
            swap(h1,h2);
        }
        auto h_values = interpolate(h1,h2,p1.y,p2.y);
        auto values = interpolate(p1.x,p2.x,p1.y,p2.y);
        for(int y = p1.y; y <= p2.y; y++){
            line.append(getPixel(values[y-p1.y],y,multiplyColorWithIntensity(color,h_values[y-p1.y])));
        }
    }
    return line;
};

std::vector<sf::VertexArray> getTriangle(sf::Vector2f l0, sf::Vector2f l1, sf::Vector2f l2, sf::Color color,float h0, float h1, float h2){
    if(l1.y < l0.y){
        swap(l0,l1);
        swap(h0,h1);
    }
    if(l2.y < l0.y){
        swap(l0,l2);
        swap(h0,h2);
    }
    if(l2.y < l1.y){
        swap(l1,l2);
        swap(h1,h2);
    }
    std::vector<sf::VertexArray> lines;
    lines.reserve(abs(l2.y - l0.y) + 1);

    auto x_02 = interpolate(l0.x,l2.x,l0.y,l2.y);
    auto h_02 = interpolate(h0,h2,l0.y,l2.y);
    auto x_01 = interpolate(l0.x,l1.x,l0.y,l1.y);
    auto h_01 = interpolate(h0,h1,l0.y,l1.y);
    auto x_12 = interpolate(l1.x,l2.x,l1.y,l2.y);
    auto h_12 = interpolate(h1,h2,l1.y,l2.y);
    
    x_01.pop_back();
    h_01.pop_back();
    for(float h:h_12){
        h_01.push_back(h);
    }
    for(float l:x_12){
        x_01.push_back(l);
    }
    for(int y = l0.y; y <= l2.y; y++){
        lines.push_back(getLine(sf::Vector2f(x_02[y -l0.y],y),sf::Vector2f(x_01[y - l0.y],y),color,h_02[y-l0.y],h_01[y-l0.y]));
    }

    return lines;
}

std::vector<sf::VertexArray> getTriangleWireFrame(Triangle triangle,std::vector<sf::Vector2f>& projected){
    std::vector<sf::VertexArray> triangle_wireframe;
    triangle_wireframe.reserve(3);
    triangle_wireframe.push_back(getLine(projected[triangle.v1],projected[triangle.v2],triangle.color,1,1));
    triangle_wireframe.push_back(getLine(projected[triangle.v2],projected[triangle.v3],triangle.color,1,1));
    triangle_wireframe.push_back(getLine(projected[triangle.v3],projected[triangle.v1],triangle.color,1,1));

    return triangle_wireframe;
}

Object getObject(const Instance& instance, float viewPort_z, Matrix& transform){
    std::vector<sf::Vector2f> projected;
    Object object;
    Matrix final_transform = transform * getTranslationMatrix(instance.translation)*getRotationMatrix(instance.angles)*getScalingMatrix(instance.scale);

    for(auto vertex:instance.model_ptr->vertices){
        
        projected.push_back(projectVector(get3DVector(final_transform*get4DVector(vertex)),viewPort_z));
    }
    for(auto triangle:instance.model_ptr->triangles){
        object.push_back(getTriangleWireFrame(triangle,projected));
    }

    return object;
}

int main() {
    sf::RenderWindow window(sf::VideoMode({WIDTH,HEIGHT}), "Rasterizer");
    sf::Clock clock;
    const sf::Font font("../assets/PoetsenOne-Regular.ttf");
    sf::Text fps(font);
    sf::Vector3f camera_pos = sf::Vector3f(0,0,0);
    std::array<float,3> camera_orientation {0,0,0};
    sf::Vector3f viewport_pos = sf::Vector3f(0,0,4);
    fps.setCharacterSize(30);
    fps.setFillColor(sf::Color::Black);
    fps.setPosition(sf::Vector2f(0,0));

    Model cube {{
                { 1,  1,  1},  // 0
                {-1,  1,  1},  // 1
                {-1, -1,  1},  // 2
                { 1, -1,  1},  // 3

                { 1,  1, -1},  // 4
                {-1,  1, -1},  // 5
                {-1, -1, -1},  // 6
                { 1, -1, -1}   // 7
            },

            {
                {0, 1, 2, sf::Color::Red},
                {0, 2, 3, sf::Color::Red},

                {4, 0, 3, sf::Color::Green},
                {4, 3, 7, sf::Color::Green},

                {5, 4, 7, sf::Color::Blue},
                {5, 7, 6, sf::Color::Blue},

                {1, 5, 6, sf::Color::Yellow},
                {1, 6, 2, sf::Color::Yellow},

                {4, 5, 1, sf::Color::Magenta},
                {4, 1, 0, sf::Color::Magenta},

                {2, 6, 7, sf::Color::Cyan},
                {2, 7, 3, sf::Color::Cyan}
            }
        };

    std::vector<Instance> instances;

    instances.push_back(
        Instance{
            &cube,
            sf::Vector3f(-7.0f,  4.0f,  4.0f),
            {20.0f, 30.0f, 10.0f},
            1.3f
        }
    );

    instances.push_back(
        Instance{
            &cube,
            sf::Vector3f(7.0f,  4.0f, 6.0f),
            {0.0f, 45.0f, 20.0f},
            1.0f
        }
    );

    instances.push_back(
        Instance{
            &cube,
            sf::Vector3f(-6.0f, -5.0f, 8.0f),
            {30.0f, 10.0f, 45.0f},
            1.0f
        }
    );

    instances.push_back(
        Instance{
            &cube,
            sf::Vector3f(6.0f, -4.0f, 15.0f),
            {45.0f, 20.0f, 0.0f},
            1.0f
        }
);

    while (window.isOpen()) {
        fps.setString(std::to_string(1/clock.getElapsedTime().asSeconds()));
        clock.restart();
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }
        window.clear(sf::Color::White);
        window.draw(fps);

        for (auto instance: instances){
            Matrix camera_transform = transposeMatrix(getRotationMatrix(camera_orientation)) * getInverseTranslationMatrix(camera_pos);
            Object object = getObject(instance,viewport_pos.z,camera_transform);
            for (auto triangle: object){
                for(auto line:triangle){
                    window.draw(line);
                }
            }
        }
        
        window.display();
    }
}
