#include <iostream>
#include <SFML/Graphics.hpp>
#include <optional>
#include <cmath>
#include <array>

constexpr int WIDTH = 800;
constexpr int HEIGHT = 800;
constexpr int VIEWPORT_W = 2;
constexpr int VIEWPORT_H = 2;
constexpr float pi = 3.14159;
using Matrix = std::array<std::array<float, 4>, 4>;
using Object = std::vector<std::vector<sf::VertexArray>>;
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
    std::array<int,3> indices;
    sf::Color color;

    Triangle():indices({0,0,0}),color(sf::Color::Black){}
    Triangle(std::array<int,3> aIndices,sf::Color aColor):indices(aIndices),color(aColor){}
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
    std::pair<sf::Vector3f,float> bounding_sphere;

    Instance(Model* Amodel_ptr,sf::Vector3f Atranslation, std::array<float,3> Aangles, float Ascale):model_ptr(Amodel_ptr),translation(Atranslation),angles(Aangles),scale(Ascale){
        sf::Vector3f center;
        for (auto vertex :model_ptr->vertices){
            center += vertex;
        }
        center *= float(model_ptr->vertices.size());

        float radius;
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

    if (!std::isfinite(screen_coords.x) ||
        !std::isfinite(screen_coords.y)) {

        std::cout << "\nBAD PROJECTION\n";
        std::cout << "3D: "
                  << vector.x << ", "
                  << vector.y << ", "
                  << vector.z << '\n';

        std::cout << "viewport: "
                  << viewport_coords.x << ", "
                  << viewport_coords.y << '\n';

        std::cout << "screen: "
                  << screen_coords.x << ", "
                  << screen_coords.y << '\n';
    }

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
    // std::cout << "i1: " << i1 << " i2: " << i2
    //       << " difference: " << i2 - i1 << '\n';
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

void getLine(sf::VertexArray& scene,sf::Vector2f p1,sf::Vector2f p2, sf::Color color,float h1, float h2){
    if(abs(p1.x - p2.x) > abs(p1.y - p2.y)){
        if(p1.x > p2.x){
            swap(p1,p2);
            swap(h1,h2);
        }
        auto h_values = interpolate(h1,h2,p1.x,p2.x);
        auto values = interpolate(p1.y,p2.y,p1.x,p2.x);
        for(int x = p1.x; x <= p2.x; x++){
            scene.append(getPixel(x,values[x-p1.x],multiplyColorWithIntensity(color,h_values[x-p1.x])));
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
            scene.append(getPixel(values[y-p1.y],y,multiplyColorWithIntensity(color,h_values[y-p1.y])));
        }
    }
};

void getTriangle(sf::VertexArray& scene, sf::Vector2f l0, sf::Vector2f l1, sf::Vector2f l2, sf::Color color,float h0, float h1, float h2){
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
        getLine(scene,sf::Vector2f(x_02[y -l0.y],y),sf::Vector2f(x_01[y - l0.y],y),color,h_02[y-l0.y],h_01[y-l0.y]);
    }
}

void getTriangleWireFrame(sf::VertexArray& scene, Triangle triangle,std::vector<sf::Vector2f>& projected){
    getLine(scene,projected[triangle.indices[0]],projected[triangle.indices[1]],triangle.color,1,1);
    getLine(scene,projected[triangle.indices[1]],projected[triangle.indices[2]],triangle.color,1,1);
    getLine(scene,projected[triangle.indices[2]],projected[triangle.indices[0]],triangle.color,1,1);
}

void getTriangleWireFrame(sf::VertexArray& scene,TempTriangleProjected triangle){
    getLine(scene,triangle.first[0],triangle.first[1],triangle.second,1,1);
    getLine(scene,triangle.first[1],triangle.first[2],triangle.second,1,1);
    getLine(scene,triangle.first[2],triangle.first[0],triangle.second,1,1);
}

std::vector<sf::Vector3f> transformVertices(std::vector<sf::Vector3f>& v, Matrix& transform){
    std::vector<sf::Vector3f> transformed_vertices;
    for(auto vertex: v){
        transformed_vertices.push_back(get3DVector(transform * get4DVector(vertex)));
    }

    return transformed_vertices;
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

std::vector<TempTriangle> getTempTriangles(const std::vector<Triangle>& triangles,const std::vector<sf::Vector3f>& transformed_vertices){
    std::vector<TempTriangle> temp_triangles;
    for (auto& triangle: triangles){
        TempTriangle temp_triangle;
        for (int i = 0; i < 3; i++){
            temp_triangle.first[i] = transformed_vertices[triangle.indices[i]];
        }
        temp_triangle.second = triangle.color;
        temp_triangles.push_back(temp_triangle);
    }
    return temp_triangles;
}

std::vector<TempTriangle> getClippedTriangles(std::vector<TempTriangle> triangles, std::pair<sf::Vector3f,float> plane){
    std::vector<TempTriangle> clipped_triangles;
    clipped_triangles.reserve(triangles.size());
    for(auto& triangle: triangles){
        float signed_distance1 = getSignedDistance(triangle.first[0],plane);
        float signed_distance2 = getSignedDistance(triangle.first[1],plane);
        float signed_distance3 = getSignedDistance(triangle.first[2],plane);

        if (signed_distance1 >= 0 && signed_distance2 >= 0 && signed_distance3 >= 0){
            clipped_triangles.push_back(triangle);
        }
        else if (signed_distance1 < 0 && signed_distance2 < 0 && signed_distance3 < 0){
            continue;
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

                clipped_triangles.push_back(temp_triangle);
            }
            else if (all_points.size() == 4){
                TempTriangle temp_triangle1;
                temp_triangle1.first[0] = all_points[0];
                temp_triangle1.first[1] = all_points[1];
                temp_triangle1.first[2] = all_points[2];
                temp_triangle1.second = triangle.second;
                clipped_triangles.push_back(temp_triangle1);

                TempTriangle temp_triangle2;
                temp_triangle2.first[0] = all_points[0];
                temp_triangle2.first[1] = all_points[3];
                temp_triangle2.first[2] = all_points[2];
                temp_triangle2.second = triangle.second;
                clipped_triangles.push_back(temp_triangle2);

            }
        }
    }

    return clipped_triangles;
}

void getObject(sf::VertexArray& scene, const Instance& instance, float viewPort_z, Matrix& transform, ClippingVolume& clipping_volume){
    std::vector<sf::Vector2f> projected;
    Matrix final_transform = transform * getTranslationMatrix(instance.translation)*getRotationMatrix(instance.angles)*getScalingMatrix(instance.scale);

    std::vector<TempTriangle> clipped_triangles;
    clipped_triangles.reserve(instance.model_ptr->triangles.size());
    std::vector<sf::Vector3f> transformed_vertices;
    transformed_vertices.reserve(instance.model_ptr->vertices.size());
    transformed_vertices = transformVertices(instance.model_ptr->vertices,final_transform);

    ObjectRelativePositionToPlane relative_position = ObjectRelativePositionToPlane::CompletelyInside;
    for(auto& plane: clipping_volume){
        if(getSignedDistance(instance.bounding_sphere.first,plane) > instance.bounding_sphere.second){
            continue;
        }
        else if(getSignedDistance(instance.bounding_sphere.first,plane) < -instance.bounding_sphere.second){
            relative_position = ObjectRelativePositionToPlane::CompletelyOutside;
            break;
        }
        else{
            relative_position = ObjectRelativePositionToPlane::InBetween;
            continue;
            }
    }

    if (relative_position == ObjectRelativePositionToPlane::InBetween){
        clipped_triangles = getTempTriangles(instance.model_ptr->triangles,transformed_vertices);

        for (auto& plane: clipping_volume){
            clipped_triangles = getClippedTriangles(clipped_triangles,plane);
        }
    }

    if(relative_position == ObjectRelativePositionToPlane::CompletelyInside){
        for(auto vertex:transformed_vertices){
            projected.push_back(projectVector(vertex,viewPort_z));
        }
        for(auto triangle:instance.model_ptr->triangles){
            getTriangleWireFrame(scene,triangle,projected);
        }
    }
    else if (relative_position == ObjectRelativePositionToPlane::InBetween){
        std::vector<TempTriangleProjected> projected_triangles;
        projected_triangles.reserve(clipped_triangles.size());
        for(auto& triangle: clipped_triangles){
            projected_triangles.push_back(projectTriangle(triangle,viewPort_z));
        }

        for (auto& triangle: projected_triangles){
            getTriangleWireFrame(scene,triangle);
        }
    }
}

int main(){
    sf::RenderWindow window(sf::VideoMode({WIDTH,HEIGHT}), "Rasterizer");
    sf::Clock clock;
    const sf::Font font("assets/PoetsenOne-Regular.ttf");
    sf::Text fps(font);
    sf::Vector3f camera_pos = sf::Vector3f(0,0,0);
    std::array<float,3> camera_orientation {0,0,0};
    sf::Vector3f viewport_pos = sf::Vector3f(0,0,1);
    fps.setCharacterSize(30);
    fps.setFillColor(sf::Color::Black);
    fps.setPosition(sf::Vector2f(0,0));
    float one_by_root_2 = 1/std::sqrt(2);
    ClippingVolume clipping_volume{
        std::pair{sf::Vector3f(0,0,1),-1},
        std::pair{sf::Vector3f(one_by_root_2,0,one_by_root_2),0},
        std::pair{sf::Vector3f(-one_by_root_2,0,one_by_root_2),0},
        std::pair{sf::Vector3f(0,one_by_root_2,one_by_root_2),0},
        std::pair{sf::Vector3f(0,-one_by_root_2,one_by_root_2),0},
    };

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
                {{0, 1, 2}, sf::Color::Red},
                {{0, 2, 3}, sf::Color::Red},

                {{4, 0, 3}, sf::Color::Green},
                {{4, 3, 7}, sf::Color::Green},

                {{5, 4, 7}, sf::Color::Blue},
                {{5, 7, 6}, sf::Color::Blue},

                {{1, 5, 6}, sf::Color::Yellow},
                {{1, 6, 2}, sf::Color::Yellow},

                {{4, 5, 1}, sf::Color::Magenta},
                {{4, 1, 0}, sf::Color::Magenta},

                {{2, 6, 7}, sf::Color::Cyan},
                {{2, 7, 3}, sf::Color::Cyan}
            }
        };

    Model triangle{{
                { 1.5f,  1.0f,  0.5f},  // 0
                {-1.2f, -0.8f,  0.2f},  // 1
                { 0.3f, -1.4f, -0.9f}   // 2
            },

            {
                {{0, 1, 2}, sf::Color::Red}
            }
        };

    std::vector<Instance> instances;

    // instances.push_back(
    //     Instance{
    //         &triangle,
    //         sf::Vector3f(-3,2.5,15),
    //         {0,0,0},
    //         0.9f
    //     }
    // );

// 1. Baseline — fully inside frustum, should look identical to before clipping existed.
instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(-3.0f, 2.5f, 15.0f),
        {20.0f, 30.0f, 10.0f},
        0.9f
    }
);

// 2. Straddles the near (z = d) plane — half in front of camera, half behind.
// Should render as a smaller, clipped shape (fewer/new triangles from the fan split),
// NOT a flipped ghost cube. This is your main regression test for the "flip" bug.
instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(0.0f, 0.0f, 1.0f),
        {15.0f, 15.0f, 0.0f},
        1.5f
    }
);

// 3. Fully behind the camera (all z < d, likely negative).
// Should completely disappear — not appear flipped/mirrored in front.
instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(4.0f, 3.0f, -5.0f),
        {0.0f, 0.0f, 0.0f},
        1.0f
    }
);

// 4. Very close and large, straddling near plane AND both side planes at once.
// Exercises multiple planes clipping the same triangle sequentially —
// the "second plane clips the already-clipped output" bug would show up here
// as either missing chunks or edges stretching across the whole screen.
instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(0.0f, 0.0f, 2.0f),
        {30.0f, 45.0f, 15.0f},
        3.0f
    }
);

// 5. Straddles the right side plane only, comfortably past the near plane.
// Isolates side-plane clipping from near-plane clipping.
instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(8.0f, 0.0f, 10.0f),
        {0.0f, 20.0f, 0.0f},
        1.2f
    }
);

// 6. Straddles the top plane only.
instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(0.0f, 6.0f, 10.0f),
        {10.0f, 0.0f, 0.0f},
        1.2f
    }
);

// 7. Small and distant but off to the side + behind near plane simultaneously —
// tests a case where the bounding-sphere early-out logic (fully in / fully out /
// straddling) has to correctly choose "straddling" and not wrongly early-exit.
instances.push_back(
    Instance{
        &cube,
        sf::Vector3f(-6.0f, -4.0f, 0.5f),
        {45.0f, 0.0f, 30.0f},
        0.7f
    }
);

    while (window.isOpen()) {
        float dt = clock.getElapsedTime().asSeconds();
        fps.setString(std::to_string(1/dt));
        clock.restart();
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }
        window.clear(sf::Color::White);
        window.draw(fps);

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

        sf::VertexArray scene (sf::PrimitiveType::Points);
        for (auto instance: instances){
            Matrix camera_transform = transposeMatrix(getRotationMatrix(camera_orientation)) * getInverseTranslationMatrix(camera_pos);
            getObject(scene,instance,viewport_pos.z,camera_transform,clipping_volume);
        }
        window.draw(scene);
        
        window.display();
    }
}
