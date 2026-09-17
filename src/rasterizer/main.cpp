#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <cmath>
#include <array>

constexpr int WIDTH = 800;
constexpr int HEIGHT = 800;
constexpr float VIEWPORT_W = 2;
constexpr float VIEWPORT_H = 2;
using DEPTH_BUFFER = std::vector<std::vector<float>>;
constexpr float pi = 3.14159;
using Matrix = std::array<std::array<float, 4>, 4>;
using Object = std::vector<std::vector<std::vector<sf::Vertex>>>;
using Range = std::array<int,2>;
using Triangle = std::pair<std::array<int,3>,sf::Color>;
using TempTriangle = std::pair<std::array<sf::Vector3f,3>,sf::Color>;
using TempTriangleProjected = std::pair<std::array<sf::Vector2f,3>,sf::Color>;
using ClippingVolume = std::array<std::pair<sf::Vector3f,float>,5>;
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

float getDotProduct(sf::Vector3f v1, sf::Vector3f v2){
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

sf::Vector3f getCrossProduct(sf::Vector3f v1, sf::Vector3f v2){
    return sf::Vector3f(v1.y*v2.z - v2.y*v1.z,-(v1.x*v2.z - v1.z*v2.x),v1.x*v2.y - v2.x*v1.y);
}

sf::Vector3f operator*(const Matrix& mat, const sf::Vector3f& vec) {
    Vector4f v (vec.x,vec.y,vec.z,1);
    Vector4f result;
    
    result.x = mat[0][0] * v.x + mat[0][1] * v.y + mat[0][2] * v.z + mat[0][3] * v.w;
    
    result.y = mat[1][0] * v.x + mat[1][1] * v.y + mat[1][2] * v.z + mat[1][3] * v.w;
    
    result.z = mat[2][0] * v.x + mat[2][1] * v.y + mat[2][2] * v.z + mat[2][3] * v.w;
    
    result.w = mat[3][0] * v.x + mat[3][1] * v.y + mat[3][2] * v.z + mat[3][3] * v.w;
    
    return sf::Vector3f(result.x/result.w, result.y/result.w, result.z/result.w);
}

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

struct Model{
    std::vector<sf::Vector3f> vertices;
    std::vector<Triangle> triangles;
};

struct Instance{
    Model* model_ptr;
    sf::Vector3f translation;
    std::array<float,3> angles;
    float scale;
    std::pair<sf::Vector3f,float> bounding_sphere;

    Instance(Model* Amodel_ptr,sf::Vector3f Atranslation, std::array<float,3> Aangles, float Ascale):model_ptr(Amodel_ptr),translation(Atranslation),angles(Aangles),scale(Ascale){
        sf::Vector3f center;
        for (auto vertex :model_ptr->vertices){
            center += vertex;
        }
        center /= float(model_ptr->vertices.size());

        float radius = 0.0f;
        float distance;
        for (auto vertex: model_ptr->vertices){
            distance = (center - vertex).length();
            if(radius < distance){
                radius = distance;
            }
        }
        bounding_sphere = {center,radius};
    }

};

sf::Vector3f canvasToViewPort(int x,int y){
    float Vx = x * (VIEWPORT_W/WIDTH);
    float Vy = y * (VIEWPORT_H/HEIGHT);
    return sf::Vector3f(Vx,Vy,1);
}

sf::Vertex getPixel(int x,int y,sf::Color color,DEPTH_BUFFER& depth_buffer,float z_inverse){
    float Sx = WIDTH/2 + x;
    float Sy = HEIGHT/2 - y;

    if(Sx < WIDTH && Sy < HEIGHT && Sx > 0 && Sy > 0){
        if(z_inverse > depth_buffer[Sx][Sy]){
            depth_buffer[Sx][Sy] = z_inverse;
            return sf::Vertex (sf::Vector2f(Sx,Sy),color);
        }
    }

    return sf::Vertex(sf::Vector2f(Sx,Sy),sf::Color::Transparent);
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

void printVector(sf::Vector3f vector){
    std::cout << "x: " << vector.x << ", y: " << vector.y << ", z: " << vector.z << std::endl;
}

sf::Vector2f projectVector(sf::Vector3f vector, float viewport_z){

    sf::Vector2f viewport_coords = {
        (vector.x * viewport_z) / vector.z,
        (vector.y * viewport_z) / vector.z
    };

    sf::Vector2f screen_coords = {
        (viewport_coords.x / VIEWPORT_W) * WIDTH,
        (viewport_coords.y / VIEWPORT_H) * HEIGHT
    };

    return screen_coords;
}

TempTriangleProjected projectTriangle(TempTriangle& triangle,float viewport_z){
    TempTriangleProjected projectedTriangle;
    for(int i = 0; i < 3; i++){
        projectedTriangle.first[i] = projectVector(triangle.first[i],viewport_z);
    }
    projectedTriangle.second = triangle.second;
    return projectedTriangle;
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

void swap(float& n1, float& n2){
    float temp = n1;
    n1 = n2;
    n2 = temp;
}

Range drawLine(sf::VertexArray& scene,DEPTH_BUFFER& depth_buffer,sf::Vector2f p1,sf::Vector2f p2, sf::Color color,float h1, float h2,float z1, float z2){
    int i_start = 0;
    sf::Vertex vertex;
    if(!(scene.getVertexCount() == 0)){
        i_start = scene.getVertexCount() - 1;
    }
    int i_end = i_start;
    if(abs(p1.x - p2.x) > abs(p1.y - p2.y)){
        if(p1.x > p2.x){
            swap(p1,p2);
            swap(h1,h2);
            swap(z1,z2);
        }
        i_end = i_start + p2.x - p1.x;
        auto h_values = interpolate(h1,h2,p1.x,p2.x);
        auto values = interpolate(p1.y,p2.y,p1.x,p2.x);
        auto z_values = interpolate(z1,z2,p1.x,p2.x);
        for(int x = p1.x; x <= p2.x; x++){
            float z = z_values[x - p1.x];
            vertex = getPixel(x,values[x-p1.x],multiplyColorWithIntensity(color,h_values[x-p1.x]),depth_buffer,z);
            if(!(vertex.color == sf::Color::Transparent)){
                scene.append(vertex);
            }
        }
    }

    else{
        if(p1.y > p2.y){
            swap(p1,p2);
            swap(h1,h2);
            swap(z1,z2);
        }
        i_end = i_start + p2.y - p1.y;
        auto h_values = interpolate(h1,h2,p1.y,p2.y);
        auto values = interpolate(p1.x,p2.x,p1.y,p2.y);
        auto z_values = interpolate(z1,z2,p1.y,p2.y);
        for(int y = p1.y; y <= p2.y; y++){
            float z = z_values[y - p1.y];
            vertex = getPixel(values[y-p1.y],y,multiplyColorWithIntensity(color,h_values[y-p1.y]),depth_buffer,z);
            if(!(vertex.color == sf::Color::Transparent)){
                scene.append(vertex);
            }
        }
    }

    return {i_start,i_end};
};

Range drawTriangle(sf::VertexArray& scene,DEPTH_BUFFER& depth_buffer,TempTriangle& triangle,float h0, float h1, float h2,float viewport_z){
    sf::Vector2f l0 = projectVector(triangle.first[0],viewport_z);
    float z0 = triangle.first[0].z;
    sf::Vector2f l1 = projectVector(triangle.first[1],viewport_z);
    float z1 = triangle.first[1].z;
    sf::Vector2f l2 = projectVector(triangle.first[2],viewport_z);
    float z2 = triangle.first[2].z;
    sf::Color color = triangle.second;

    int i_start = scene.getVertexCount() - 1;

    if(l1.y < l0.y){
        swap(l0,l1);
        swap(h0,h1);
        swap(z0,z1);
    }
    if(l2.y < l0.y){
        swap(l0,l2);
        swap(h0,h2);
        swap(z0,z2);
    }
    if(l2.y < l1.y){
        swap(l1,l2);
        swap(h1,h2);
        swap(z1,z2);
    }
    std::vector<std::vector<sf::Vertex>> lines;
    lines.reserve(abs(l2.y - l0.y) + 1);

    auto x_02 = interpolate(l0.x,l2.x,l0.y,l2.y);
    auto h_02 = interpolate(h0,h2,l0.y,l2.y);
    auto x_01 = interpolate(l0.x,l1.x,l0.y,l1.y);
    auto h_01 = interpolate(h0,h1,l0.y,l1.y);
    auto x_12 = interpolate(l1.x,l2.x,l1.y,l2.y);
    auto h_12 = interpolate(h1,h2,l1.y,l2.y);
    auto z_02 = interpolate(1/z0,1/z2,l0.y,l2.y);
    auto z_01 = interpolate(1/z0,1/z1,l0.y,l1.y);
    auto z_12 = interpolate(1/z1,1/z2,l1.y,l2.y);
    
    x_01.pop_back();
    h_01.pop_back();
    z_01.pop_back();
    for(float h:h_12){
        h_01.push_back(h);
    }
    for(float l:x_12){
        x_01.push_back(l);
    }
    for (float z:z_12){
        z_01.push_back(z);
    }

    int i_end = i_start + (l2.y - l0.y);
    for(int y = l0.y; y <= l2.y; y++){
        drawLine(scene,depth_buffer,sf::Vector2f(x_02[y -l0.y],y),sf::Vector2f(x_01[y - l0.y],y),color,h_02[y-l0.y],h_01[y-l0.y],z_02[y - l0.y],z_01[y-l0.y]);
    }

    return {i_start,i_end};
}

std::array<Range,3> drawTriangleWireFrame(sf::VertexArray& scene,DEPTH_BUFFER& depth_buffer,TempTriangleProjected triangle){
    std::array<Range,3> ranges;
    ranges[0] = drawLine(scene,depth_buffer,triangle.first[0],triangle.first[1],triangle.second,1,1,1,1);
    ranges[1] = drawLine(scene,depth_buffer,triangle.first[1],triangle.first[2],triangle.second,1,1,1,1);
    ranges[2] = drawLine(scene,depth_buffer,triangle.first[2],triangle.first[0],triangle.second,1,1,1,1);
    
    return ranges;
}

float getSignedDistance(sf::Vector3f point,std::pair<sf::Vector3f,float> plane){
    return getDotProduct(point,plane.first) + plane.second;
}

sf::Vector3f getIntersection(sf::Vector3f A, sf::Vector3f B, std::pair<sf::Vector3f,float> plane){
    float t = (-plane.second - getDotProduct(plane.first,A))/getDotProduct(plane.first,B-A);
    return A + t*(B-A);
}

enum ObjectRelativePositionToPlane{
    CompletelyInside,
    CompletelyOutside,
    InBetween
};

TempTriangle getTempTriangle(Triangle& triangle, std::vector<sf::Vector3f>& transformed_vertices){
    TempTriangle temp_triangle;
    temp_triangle.first[0] = transformed_vertices[triangle.first[0]];
    temp_triangle.first[1] = transformed_vertices[triangle.first[1]];
    temp_triangle.first[2] = transformed_vertices[triangle.first[2]];
    temp_triangle.second = triangle.second;

    return temp_triangle;
}

int clipTriangle(TempTriangle& triangle, std::pair<sf::Vector3f,float> plane, TempTriangle out[2]){

    float signed_distance1 = getSignedDistance(triangle.first[0],plane);
    float signed_distance2 = getSignedDistance(triangle.first[1],plane);
    float signed_distance3 = getSignedDistance(triangle.first[2],plane);
    int number_of_triangles = 0;

    if (signed_distance1 >= 0 && signed_distance2 >= 0 && signed_distance3 >= 0){
        out[0] = triangle;
        number_of_triangles = 1;
    }
    else if (signed_distance1 < 0 && signed_distance2 < 0 && signed_distance3 < 0){
        number_of_triangles = 0;
    }
    else{
        std::vector<sf::Vector3f> all_points;
        all_points.reserve(4);
        for(int i = 0; i < 3; i++){
            sf::Vector3f current_vertex = triangle.first[i];
            sf::Vector3f next_vertex = triangle.first[(i+1)%3];

            float d_current = getSignedDistance(current_vertex,plane);
            float d_next = getSignedDistance(next_vertex,plane);

            if (d_current >= 0){
                all_points.push_back(current_vertex);
                if(d_next < 0){
                    all_points.push_back(getIntersection(current_vertex,next_vertex,plane));
                }
            }
            else{
                if(d_next >= 0){
                    all_points.push_back(getIntersection(current_vertex,next_vertex,plane));
                }
            }
        }

        if (all_points.size() == 3){
            TempTriangle temp_triangle;
            temp_triangle.first[0] = all_points[0];
            temp_triangle.first[1] = all_points[1];
            temp_triangle.first[2] = all_points[2];
            temp_triangle.second = triangle.second;

            out[0] = temp_triangle;
            number_of_triangles = 1;
        }
        else if (all_points.size() == 4){
            TempTriangle temp_triangle1;
            temp_triangle1.first[0] = all_points[0];
            temp_triangle1.first[1] = all_points[1];
            temp_triangle1.first[2] = all_points[2];
            temp_triangle1.second = triangle.second;
            out[0] = temp_triangle1;

            TempTriangle temp_triangle2;
            temp_triangle2.first[0] = all_points[0];
            temp_triangle2.first[1] = all_points[3];
            temp_triangle2.first[2] = all_points[2];
            temp_triangle2.second = triangle.second;
            out[1] = temp_triangle2;

            number_of_triangles = 2;
        }
    }

    return number_of_triangles;
}

bool isTriangleVisible(const TempTriangle& triangle) {
    const auto& a = triangle.first[0];
    const auto& b = triangle.first[1];
    const auto& c = triangle.first[2];

    sf::Vector3f normal = getCrossProduct(b - a, c - a);
    sf::Vector3f centre = (a + b + c) / 3.f;

    return getDotProduct(normal, -centre) > 0.f;
}

int main(){
    sf::RenderWindow window(sf::VideoMode({WIDTH,HEIGHT}), "Rasterizer");
    DEPTH_BUFFER depth_buffer;
    depth_buffer.resize(WIDTH);
    for(auto& col: depth_buffer){
        col.resize(HEIGHT,0);
    }
    sf::Clock fps_clock;
    const sf::Font font("assets/PoetsenOne-Regular.ttf");
    sf::Text fps(font);
    fps.setCharacterSize(30);
    fps.setFillColor(sf::Color::Black);
    fps.setPosition(sf::Vector2f(0,0));
    fps.setString("0");
    sf::Clock clock;

    sf::Vector3f camera_pos = sf::Vector3f(0,0,0);
    std::array<float,3> camera_orientation {0,0,0};
    sf::Vector3f viewport_pos = sf::Vector3f(0,0,1);
    float one_by_root_2 = 1/std::sqrt(2);
    ClippingVolume clipping_volume{
        std::pair{sf::Vector3f(0,0,1),-1},
        std::pair{sf::Vector3f(one_by_root_2,0,one_by_root_2),0},
        std::pair{sf::Vector3f(-one_by_root_2,0,one_by_root_2),0},
        std::pair{sf::Vector3f(0,one_by_root_2,one_by_root_2),0},
        std::pair{sf::Vector3f(0,-one_by_root_2,one_by_root_2),0},
    };

    Model cube {{
        // Front face (z = +1), CCW from outside (+z looking toward -z)
        { 1,  1,  1},  // 0  top-right-front
        {-1,  1,  1},  // 1  top-left-front
        {-1, -1,  1},  // 2  bottom-left-front
        { 1, -1,  1},  // 3  bottom-right-front

        // Back face (z = -1)
        { 1,  1, -1},  // 4  top-right-back
        {-1,  1, -1},  // 5  top-left-back
        {-1, -1, -1},  // 6  bottom-left-back
        { 1, -1, -1}   // 7  bottom-right-back
    },
    {
        // +Z
        {{0, 1, 2}, sf::Color::Red},
        {{0, 2, 3}, sf::Color::Red},

        // +X
        {{4, 0, 3}, sf::Color::Green},
        {{4, 3, 7}, sf::Color::Green},

        // -Z
        {{5, 7, 6}, sf::Color::Blue},
        {{5, 4, 7}, sf::Color::Blue},

        // -X
        {{1, 6, 2}, sf::Color::Yellow},
        {{1, 5, 6}, sf::Color::Yellow},

        // +Y
        {{1, 4, 5}, sf::Color::Magenta},
        {{1, 0, 4}, sf::Color::Magenta},

        // -Y
        {{2, 7, 3}, sf::Color::Cyan},
        {{2, 6, 7}, sf::Color::Cyan}
    }};

    Model triangle{{
        { 1.5f,  1.0f,  0.5f},  // 0
        {-1.2f, -0.8f,  0.2f},  // 1
        { 0.3f, -1.4f, -0.9f}   // 2
    },
    {
        {{0, 1, 2}, sf::Color::Red}
    }};

    std::vector<std::array<Range,3>> wireframe_ranges;
    std::vector<Range> ranges;
    std::vector<Instance> instances;

    // instances.push_back(
    //     Instance{
    //         &triangle,
    //         sf::Vector3f(-3,2.5,15),
    //         {0,0,0},
    //         0.9f
    //     }
    // );

instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(-3.0f, 2.5f, 15.0f),
        {20.0f, 30.0f, 10.0f},
        0.9f
    }
);

instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(0.0f, 0.0f, 1.0f),
        {15.0f, 15.0f, 0.0f},
        1.5f
    }
);

instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(4.0f, 3.0f, -5.0f),
        {0.0f, 0.0f, 0.0f},
        1.0f
    }
);

instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(0.0f, 0.0f, 2.0f),
        {30.0f, 45.0f, 15.0f},
        3.0f
    }
);

instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(8.0f, 0.0f, 10.0f),
        {0.0f, 20.0f, 0.0f},
        1.2f
    }
);

instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(0.0f, 6.0f, 10.0f),
        {10.0f, 0.0f, 0.0f},
        1.2f
    }
);

instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(-6.0f, -4.0f, 0.5f),
        {45.0f, 0.0f, 30.0f},
        0.7f
    }
);

    sf::VertexArray scene(sf::PrimitiveType::Points);
    std::vector<TempTriangle> triangle_buff1, triangle_buff2;
    std::vector<sf::Vector3f> vertices_buff;
    std::vector<sf::Vector2f> projected_triangles_buff;
    std::vector<TempTriangleProjected> projected_temp_triangles_buff;

    while (window.isOpen()) {
        for(auto& col:depth_buffer){
            for(auto& z: col){
                z = 0;
            }
        }

        float dt = fps_clock.getElapsedTime().asSeconds();
        fps_clock.restart();

        if(clock.getElapsedTime().asSeconds() >= 1){
            fps.setString(std::to_string(1/dt));
            clock.restart();
        }

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }
        window.clear(sf::Color::White);

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)){
            camera_pos.x += 5*dt;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)){
            camera_pos.x -= 5*dt;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)){
            camera_pos.y += 5*dt;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)){
            camera_pos.y -= 5*dt;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)){
            camera_pos.z += 5*dt;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)){
            camera_pos.z -= 5*dt;
        }

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)){
            camera_orientation[0] += 10*dt;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::G)){
            camera_orientation[1] += 10*dt;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::H)){
            camera_orientation[2] += 10*dt;
        }

        Matrix camera_transform = transposeMatrix(getRotationMatrix(camera_orientation)) * getInverseTranslationMatrix(camera_pos);
        for (auto instance: instances){
            Matrix final_transform = camera_transform * getTranslationMatrix(instance.translation)*getRotationMatrix(instance.angles)*getScalingMatrix(instance.scale);

            ObjectRelativePositionToPlane relative_position = ObjectRelativePositionToPlane::CompletelyInside;
            
            for(auto& plane: clipping_volume){
                float distance = getSignedDistance(final_transform*instance.bounding_sphere.first,plane);
                float radius = instance.scale * instance.bounding_sphere.second;
                if(distance > radius){
                    continue;
                }
                else if(distance < -radius){
                    relative_position = ObjectRelativePositionToPlane::CompletelyOutside;
                    break;
                }
                else{
                    relative_position = ObjectRelativePositionToPlane::InBetween;
                    continue;
                    }
            }

            for(auto vertex: instance.model_ptr->vertices){
                vertices_buff.push_back(final_transform*vertex);
            }

            triangle_buff1.clear();
            for(auto& triangle:instance.model_ptr->triangles){
                TempTriangle temp_triangle = getTempTriangle(triangle,vertices_buff);
                // sf::Vector3f camera_orientation_from_origin = sf::Vector3f(std::cos(camera_orientation[0])*std::sin(camera_orientation[1]),std::sin(camera_orientation[0]),std::cos(camera_orientation[1])*std::cos(camera_orientation[0]));
                // sf::Vector3f camera_vector = camera_orientation_from_origin + camera_pos;
                if(isTriangleVisible(temp_triangle)){
                    triangle_buff1.push_back(temp_triangle);
                }
            }

            if(relative_position == ObjectRelativePositionToPlane::CompletelyInside){
                // for(auto& triangle: triangle_buff1){
                //     projected_temp_triangles_buff.push_back(projectTriangle(triangle,viewport_pos.z));
                // }

                for (auto& triangle: triangle_buff1){
                    ranges.push_back(drawTriangle(scene,depth_buffer,triangle,1,1,1,viewport_pos.z));
                }

            }
            else if (relative_position == ObjectRelativePositionToPlane::InBetween){

                for (auto& plane: clipping_volume){
                    for (auto& triangle: triangle_buff1){
                        TempTriangle out[2];
                        int n = clipTriangle(triangle,plane,out);
                        for(int i = 0; i < n; i++){
                            triangle_buff2.push_back(out[i]);
                        }
                    }
                    swap(triangle_buff1,triangle_buff2);
                    triangle_buff2.clear();
                }
                // for(auto& triangle: triangle_buff1){
                //     projected_temp_triangles_buff.push_back(projectTriangle(triangle,viewport_pos.z));
                // }

                for (auto& triangle: triangle_buff1){
                    ranges.push_back(drawTriangle(scene,depth_buffer,triangle,1,1,1,viewport_pos.z));
                }
            }

            vertices_buff.clear();
            projected_triangles_buff.clear();
            projected_temp_triangles_buff.clear();
        }

        window.draw(scene);
        window.draw(fps);

        wireframe_ranges.clear();
        scene.clear();
        
        window.display();
    }
}