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

sf::Color operator*(const sf::Color& color, const float intensity){
    auto scale = [intensity](std::uint8_t channel){return static_cast<std::uint8_t>(std::clamp(channel*intensity,0.0f,255.0f));};
    return sf::Color(scale(color.r),scale(color.g),scale(color.b));
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

struct Triangle{
    std::array<int,3> indices;
    sf::Color color;
    std::array<sf::Vector3f,3> normals{sf::Vector3f(0,0,0),sf::Vector3f(0,0,0),sf::Vector3f(0,0,0)};
};

struct TempTriangle{
    std::array<sf::Vector3f,3> vertices;
    sf::Color color;
    std::array<sf::Vector3f,3> normals{sf::Vector3f(0,0,0),sf::Vector3f(0,0,0),sf::Vector3f(0,0,0)};
};

struct Model{
    std::vector<sf::Vector3f> vertices;
    std::vector<Triangle> triangles;
};


// MADE BY AI (I DIDN'T BOTHERED HAVING A HUGE SPHERE MESH IN THE CODE)
// Builds a UV sphere centred at the origin.  The latitude and longitude values
// control the number of horizontal rings and vertical slices respectively.
Model createSphereModel(float radius, int latitudeSegments, int longitudeSegments,
                        sf::Color color) {
    latitudeSegments = std::max(latitudeSegments, 2);
    longitudeSegments = std::max(longitudeSegments, 3);

    Model sphere;
    std::vector<sf::Vector3f> vertexNormals;  // parallel to sphere.vertices
    const size_t vertexCount = 2 + (latitudeSegments - 1) * longitudeSegments;
    sphere.vertices.reserve(vertexCount);
    vertexNormals.reserve(vertexCount);
    sphere.triangles.reserve(2 * longitudeSegments * (latitudeSegments - 1));

    const float twoPi = 2.0f * pi;
    sphere.vertices.push_back({0.0f, radius, 0.0f});       // north pole
    vertexNormals.push_back({0.0f, 1.0f, 0.0f});

    for (int latitude = 1; latitude < latitudeSegments; ++latitude) {
        const float phi = pi * static_cast<float>(latitude) / latitudeSegments;
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);
        const float ringRadius = radius * sinPhi;
        const float y = radius * cosPhi;

        for (int longitude = 0; longitude < longitudeSegments; ++longitude) {
            const float theta = twoPi * static_cast<float>(longitude) / longitudeSegments;
            const float cosTheta = std::cos(theta);
            const float sinTheta = std::sin(theta);
            sphere.vertices.push_back({ringRadius * cosTheta, y, ringRadius * sinTheta});
            vertexNormals.push_back({sinPhi * cosTheta, cosPhi, sinPhi * sinTheta});
        }
    }

    const int southPole = static_cast<int>(sphere.vertices.size());
    sphere.vertices.push_back({0.0f, -radius, 0.0f});      // south pole
    vertexNormals.push_back({0.0f, -1.0f, 0.0f});

    auto addTriangle = [&](int a, int b, int c) {
        sphere.triangles.push_back({{a, b, c}, color,
                                    {vertexNormals[a], vertexNormals[b], vertexNormals[c]}});
    };

    // Top cap.
    for (int longitude = 0; longitude < longitudeSegments; ++longitude) {
        const int next = (longitude + 1) % longitudeSegments;
        addTriangle(0, 1 + next, 1 + longitude);
    }

    // Join neighbouring rings.  The winding faces outwards for back-face culling.
    for (int latitude = 0; latitude < latitudeSegments - 2; ++latitude) {
        const int upperRing = 1 + latitude * longitudeSegments;
        const int lowerRing = upperRing + longitudeSegments;
        for (int longitude = 0; longitude < longitudeSegments; ++longitude) {
            const int next = (longitude + 1) % longitudeSegments;
            addTriangle(upperRing + longitude, upperRing + next, lowerRing + next);
            addTriangle(upperRing + longitude, lowerRing + next, lowerRing + longitude);
        }
    }

    // Bottom cap.
    const int lastRing = southPole - longitudeSegments;
    for (int longitude = 0; longitude < longitudeSegments; ++longitude) {
        const int next = (longitude + 1) % longitudeSegments;
        addTriangle(southPole, lastRing + longitude, lastRing + next);
    }

    return sphere;
}

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

float magnitude(sf::Vector3f v){
    return sqrt(getDotProduct(v,v));
}

sf::Vector3f normalize(sf::Vector3f v){
    float mag = magnitude(v);
    if(!(mag == 0)){
        return (1/mag) * v;
    }
    return sf::Vector3f(-1,-1,-1);
}

sf::Vector3f reflect(sf::Vector3f v, sf::Vector3f n){
    return 2.f*normalize(n)*getDotProduct(normalize(n),v) - v;
}

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

std::array<float,3> computeLighting(TempTriangle& triangle,std::vector<Light>& lights,Matrix& camera_transform, Matrix& camera_rotation){
    sf::Vector3f L;

    std::array<float,3> intensities{0.0f,0.0f,0.0f};
    for(int j = 0; j < 3; j++){
        for (auto& light: lights){
            if(light.type == LightType::Ambient){
                intensities[j] += light.intensity;
            }
            else{
                if (light.type == LightType::Point){
                    L = normalize(camera_transform * light.dirOrPos - triangle.vertices[j]);
                }
                else if (light.type == LightType::Directional){
                    L = normalize(camera_rotation * light.dirOrPos);
                }
                intensities[j] += light.intensity * std::max(0.0f,getDotProduct(triangle.normals[j],L));
            }
        }
    }

    return intensities;
}

sf::Vector3f canvasToViewPort(int x,int y){
    float Vx = x * (VIEWPORT_W/WIDTH);
    float Vy = y * (VIEWPORT_H/HEIGHT);
    return sf::Vector3f(Vx,Vy,1);
}

sf::Vertex getPixel(int x,int y,sf::Color color,DEPTH_BUFFER& depth_buffer,float z_inverse){
    float Sx = WIDTH/2 + x;
    float Sy = HEIGHT/2 - y;

    if(Sx < WIDTH && Sy < HEIGHT && Sx > 0 && Sy > 0){
        if(z_inverse > depth_buffer[Sy][Sx]){
            depth_buffer[Sy][Sx] = z_inverse;
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
        projectedTriangle.first[i] = projectVector(triangle.vertices[i],viewport_z);
    }
    projectedTriangle.second = triangle.color;
    return projectedTriangle;
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
            vertex = getPixel(x,values[x-p1.x],color*h_values[x-p1.x],depth_buffer,z);
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
            vertex = getPixel(values[y-p1.y],y,color*h_values[y-p1.y],depth_buffer,z);
            if(!(vertex.color == sf::Color::Transparent)){
                scene.append(vertex);
            }
        }
    }

    return {i_start,i_end};
}

Range drawTriangle(sf::VertexArray& scene,DEPTH_BUFFER& depth_buffer,TempTriangle& triangle,float h0, float h1, float h2,float viewport_z){
    // Project the vertices and get the z values stored in a separate variable for ease.
    sf::Vector2f l0 = projectVector(triangle.vertices[0],viewport_z);
    float z0 = triangle.vertices[0].z;
    sf::Vector2f l1 = projectVector(triangle.vertices[1],viewport_z);
    float z1 = triangle.vertices[1].z;
    sf::Vector2f l2 = projectVector(triangle.vertices[2],viewport_z);
    float z2 = triangle.vertices[2].z;
    sf::Color color = triangle.color;

    // starting the counter for range
    int i_start = scene.getVertexCount() - 1;


    // swapping the vertices so that the y is in increasing order of l0 < l1 < l2
    // also since h and z are technically properties of these vertices we swap them along with these vertices
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

    // pre caluclate the inverse of z
    float iz0 = 1.0f / z0;
    float iz1 = 1.0f / z1;
    float iz2 = 1.0f / z2;

    // correctly rounding off the float values so we don't get lines shooting from the edges of the cubes
    // though the change is barely visible
    int y_start = std::ceil(l0.y);
    int y_end = std::floor(l2.y);
    // int y_start = l0.y;
    // int y_end = l2.y;

    // in some cases like l0.y = 10.2 and l2.y = 10.8 y_start and y_end will end up to be 11 and 10 because of 
    // std::ceil and std::floor so we do not draw them.

    if (y_end < y_start) {
        return {i_start, i_start - 1};
    }

    // precalculating the change in y at each edge of the triangle
    float dy_02 = l2.y - l0.y;
    float dy_01 = l1.y - l0.y;
    float dy_12 = l2.y - l1.y;

    // precalculating slope for every edge of the triangle along with slope for z reciporocal values too
    float dx_02  = (dy_02 != 0.0f) ? (l2.x - l0.x) / dy_02 : 0.0f;
    float diz_02 = (dy_02 != 0.0f) ? (iz2  - iz0 ) / dy_02 : 0.0f;
    float dh_02 = (dy_02 != 0.0f) ? (h2 - h0) / dy_02 : 0.0f;
    float dx_01  = (dy_01 != 0.0f) ? (l1.x - l0.x) / dy_01 : 0.0f;
    float diz_01 = (dy_01 != 0.0f) ? (iz1  - iz0 ) / dy_01 : 0.0f;
    float dh_01 = (dy_02 != 0.0f) ? (h1 - h0) / dy_01 : 0.0f;
    float dx_12  = (dy_12 != 0.0f) ? (l2.x - l1.x) / dy_12 : 0.0f;
    float diz_12 = (dy_12 != 0.0f) ? (iz2  - iz1 ) / dy_12 : 0.0f;
    float dh_12 = (dy_02 != 0.0f) ? (h2 - h1) / dy_12 : 0.0f;

    // setting up the x and z values for both ends of the line hence the start and end prefixes
    float x_start, x_end, z_start, z_end, h_start, h_end;
    for(int y = y_start; y <= y_end; y++){

        // we take fy to not get any wierd behaviour in case of int and float operations
        float fy = (float)(y);

        // finding the x_start
        float d02 = fy - l0.y;
        x_start = l0.x +  d02 * dx_02;
        z_start = iz0 + d02  * diz_02;
        h_start = h0 + d02 * dh_02;
        
        // since we have two edges for x_end we take two cases and calculate x_end
        if(fy >= l1.y){
            float d12 = fy - l1.y;
            x_end = l1.x + d12 * dx_12;
            z_end = iz1 + d12 * diz_12;
            h_end = h1 + d12 * dh_12;
        }
        else{
            x_end = l0.x + d02 * dx_01;
            z_end = iz0 + d02 * diz_01;
            h_end = h0 + d02 * dh_01;
        }

        // we don't know which side is left or right since our for loop will need that, we create temporary
        // variable and swap them if needed and use those in the for loop
        float x_left = x_start, z_left = z_start, h_left = h_start;
        float x_right = x_end, z_right = z_end, h_right = h_end;

        // swap the variables to follow our for loop
        if(x_end < x_start){
            swap(x_left,x_right);
            swap(z_left,z_right);
            swap(h_left,h_right);
        }

        // again using std::ceil and std::floor to take out any weird shooting lines from the edges
        int pixel_x_start = std::ceil(x_left);
        int pixel_x_end   = std::floor(x_right);

        // finding the slope of z from x_left to x_right
        float d_x = x_right - x_left;
        float d_zi = (d_x != 0.0f) ? (z_right - z_left) / (d_x) : 0.0f;
        float d_h = (d_x != 0.0f) ? (h_right - h_left) / (d_x) : 0.0f;
        
        // finding z reciprocal for the first step.
        const int half_w = WIDTH / 2;

        // clamping the x values to screen dimensions
        pixel_x_start = std::max(pixel_x_start, -half_w);
        pixel_x_end   = std::min(pixel_x_end, half_w - 1);

        if (pixel_x_end < pixel_x_start) {
            continue;
        }

        float iz = z_left + (pixel_x_start - x_left) * d_zi;
        float h = h_left + (pixel_x_start - x_left) * d_h;

        for (int x = pixel_x_start; x <= pixel_x_end; ++x, iz += d_zi, h += d_h) {
            sf::Vertex vertex = getPixel(x,y,color*h,depth_buffer,iz);
            if(!(vertex.color == sf::Color::Transparent)){
                scene.append(vertex);
            }
        }
    }

    // finding the end of the range
    int i_end = scene.getVertexCount() - 1;

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

TempTriangle getTempTriangle(Triangle& triangle, std::vector<sf::Vector3f>& transformed_vertices,Matrix& rotation_matrix){
    TempTriangle temp_triangle;
    temp_triangle.vertices[0] = transformed_vertices[triangle.indices[0]];
    temp_triangle.vertices[1] = transformed_vertices[triangle.indices[1]];
    temp_triangle.vertices[2] = transformed_vertices[triangle.indices[2]];
    temp_triangle.color = triangle.color;
    for(int i = 0; i < 3; i++){
        temp_triangle.normals[i] = rotation_matrix * triangle.normals[i];
    }

    return temp_triangle;
}

int clipTriangle(TempTriangle& triangle, std::pair<sf::Vector3f,float> plane, TempTriangle out[2]){

    float signed_distance1 = getSignedDistance(triangle.vertices[0],plane);
    float signed_distance2 = getSignedDistance(triangle.vertices[1],plane);
    float signed_distance3 = getSignedDistance(triangle.vertices[2],plane);
    int number_of_triangles = 0;

    if (signed_distance1 >= 0 && signed_distance2 >= 0 && signed_distance3 >= 0){
        out[0] = triangle;
        number_of_triangles = 1;
    }
    else if (signed_distance1 < 0 && signed_distance2 < 0 && signed_distance3 < 0){
        number_of_triangles = 0;
    }
    else{
        std::array<sf::Vector3f,4> all_points;
        int j = 0;
        for(int i = 0; i < 3; i++){
            sf::Vector3f current_vertex = triangle.vertices[i];
            sf::Vector3f next_vertex = triangle.vertices[(i+1)%3];

            float d_current = getSignedDistance(current_vertex,plane);
            float d_next = getSignedDistance(next_vertex,plane);

            if (d_current >= 0){
                all_points[j] = current_vertex;
                j++;
                if(d_next < 0){
                    all_points[j] = getIntersection(current_vertex,next_vertex,plane);
                    j++;
                }
            }
            else{
                if(d_next >= 0){
                    all_points[j] = getIntersection(current_vertex,next_vertex,plane);
                    j++;
                }
            }
        }

        if (j == 3){
            TempTriangle temp_triangle;
            temp_triangle.vertices[0] = all_points[0];
            temp_triangle.vertices[1] = all_points[1];
            temp_triangle.vertices[2] = all_points[2];
            temp_triangle.color = triangle.color;
            temp_triangle.normals = triangle.normals;

            out[0] = temp_triangle;
            number_of_triangles = 1;
        }
        else if (j == 4){
            TempTriangle temp_triangle1;
            temp_triangle1.vertices[0] = all_points[0];
            temp_triangle1.vertices[1] = all_points[1];
            temp_triangle1.vertices[2] = all_points[2];
            temp_triangle1.color = triangle.color;
            temp_triangle1.normals = triangle.normals;
            out[0] = temp_triangle1;

            TempTriangle temp_triangle2;
            temp_triangle2.vertices[0] = all_points[0];
            temp_triangle2.vertices[1] = all_points[2];
            temp_triangle2.vertices[2] = all_points[3];
            temp_triangle2.color = triangle.color;
            temp_triangle2.normals = triangle.normals;
            out[1] = temp_triangle2;

            number_of_triangles = 2;
        }
    }

    return number_of_triangles;
}

bool isTriangleVisible(const TempTriangle& triangle){
    const auto& a = triangle.vertices[0];
    const auto& b = triangle.vertices[1];
    const auto& c = triangle.vertices[2];

    sf::Vector3f normal = getCrossProduct(b - a, c - a);
    sf::Vector3f centre = (a + b + c) / 3.f;

    return getDotProduct(normal, -centre) > 0.f;
}

int main(){
    sf::RenderWindow window(sf::VideoMode({WIDTH,HEIGHT}), "Rasterizer");
    DEPTH_BUFFER depth_buffer;
    depth_buffer.resize(HEIGHT);
    for(auto& col: depth_buffer){
        col.resize(WIDTH,0);
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

    const sf::Vector3f posX { 1,  0,  0};
    const sf::Vector3f negX {-1,  0,  0};
    const sf::Vector3f posY { 0,  1,  0};
    const sf::Vector3f negY { 0, -1,  0};
    const sf::Vector3f posZ { 0,  0,  1};
    const sf::Vector3f negZ { 0,  0, -1};

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
            {{0, 1, 2}, sf::Color::Red,     {posZ, posZ, posZ}},
            {{0, 2, 3}, sf::Color::Red,     {posZ, posZ, posZ}},

            // +X
            {{4, 0, 3}, sf::Color::Green,   {posX, posX, posX}},
            {{4, 3, 7}, sf::Color::Green,   {posX, posX, posX}},

            // -Z
            {{5, 7, 6}, sf::Color::Blue,    {negZ, negZ, negZ}},
            {{5, 4, 7}, sf::Color::Blue,    {negZ, negZ, negZ}},

            // -X
            {{1, 6, 2}, sf::Color::Yellow,  {negX, negX, negX}},
            {{1, 5, 6}, sf::Color::Yellow,  {negX, negX, negX}},

            // +Y
            {{1, 4, 5}, sf::Color::Magenta, {posY, posY, posY}},
            {{1, 0, 4}, sf::Color::Magenta, {posY, posY, posY}},

            // -Y
            {{2, 7, 3}, sf::Color::Cyan,    {negY, negY, negY}},
            {{2, 6, 7}, sf::Color::Cyan,    {negY, negY, negY}}
        }
    };
    Model triangle{{
        { 1.5f,  1.0f,  0.5f},  // 0
        {-1.2f, -0.8f,  0.2f},  // 1
        { 0.3f, -1.4f, -0.9f}   // 2
    },
    {
        {{0, 1, 2}, sf::Color::Red}
    }};

    // A visual marker only; it does not participate in the lighting calculation.
    const sf::Color lightMarkerColor(255, 230, 0);
    Model pointLightMarker{cube.vertices, {
        {{0, 1, 2}, lightMarkerColor}, {{0, 2, 3}, lightMarkerColor},
        {{4, 0, 3}, lightMarkerColor}, {{4, 3, 7}, lightMarkerColor},
        {{5, 7, 6}, lightMarkerColor}, {{5, 4, 7}, lightMarkerColor},
        {{1, 6, 2}, lightMarkerColor}, {{1, 5, 6}, lightMarkerColor},
        {{1, 4, 5}, lightMarkerColor}, {{1, 0, 4}, lightMarkerColor},
        {{2, 7, 3}, lightMarkerColor}, {{2, 6, 7}, lightMarkerColor}
    }};

    // Explicit low-poly sphere, kept alongside createSphereModel() for comparison.
    const sf::Color sphereBaseColor(80, 170, 255);
    Model sphere = createSphereModel(1.0f, 12, 16, sf::Color(80, 170, 255));
    sf::Vector3f pointLightPosition(5.0f, 0.0f, -4.0f);
    std::vector<Light> lights;
    lights.emplace_back(Point,0.6,pointLightPosition);
    lights.emplace_back(Ambient,0.2);
    lights.emplace_back(Directional,0.4,sf::Vector3f(1,4,4));

    std::vector<std::array<Range,3>> wireframe_ranges;
    std::vector<Range> ranges;
    std::vector<Instance> instances;
    instances.push_back(Instance{&pointLightMarker, pointLightPosition,
                                 {0.0f, 0.0f, 0.0f}, 0.12f});
    instances.push_back(Instance{&cube, {-3.0f,  2.5f, 15.0f}, {20.0f, 30.0f, 10.0f}, 0.9f});
    instances.push_back(Instance{&sphere, { 0.0f,  0.0f,  4.0f}, {15.0f, 15.0f,  0.0f}, 1.5f});
    instances.push_back(Instance{&cube, { 4.0f,  3.0f, -5.0f}, { 0.0f,  0.0f,  0.0f}, 1.0f});
    instances.push_back(Instance{&cube, { 0.0f,  0.0f,  2.0f}, {30.0f, 45.0f, 15.0f}, 3.0f});
    instances.push_back(Instance{&cube, { 8.0f,  0.0f, 10.0f}, { 0.0f, 20.0f,  0.0f}, 1.2f});
    instances.push_back(Instance{&cube, { 0.0f,  6.0f, 10.0f}, {10.0f,  0.0f,  0.0f}, 1.2f});
    instances.push_back(Instance{&cube, {-6.0f, -4.0f,  0.5f}, {45.0f,  0.0f, 30.0f}, 0.7f});

    sf::VertexArray scene(sf::PrimitiveType::Points);
    std::vector<TempTriangle> triangle_buff1, triangle_buff2;
    std::vector<sf::Vector3f> vertices_buff;
    std::vector<sf::Vector2f> projected_triangles_buff;
    std::vector<TempTriangleProjected> projected_temp_triangles_buff;

    while (window.isOpen()) {
        float dt = fps_clock.getElapsedTime().asSeconds();
        fps_clock.restart();

        scene.resize(0);
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

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) camera_pos.x += 5 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))  camera_pos.x -= 5 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))    camera_pos.y += 5 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))  camera_pos.y -= 5 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))     camera_pos.z += 5 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))     camera_pos.z -= 5 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F))     camera_orientation[0] += 10 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::G))     camera_orientation[1] += 10 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::H))     camera_orientation[2] += 10 * dt;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L)) pointLightPosition.x += 5 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J))  pointLightPosition.x -= 5 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::I))    pointLightPosition.y += 5 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K))  pointLightPosition.y -= 5 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::O))     pointLightPosition.z += 5 * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::P))     pointLightPosition.z -= 5 * dt;

        instances[0].translation = pointLightPosition;
        lights[0].dirOrPos = pointLightPosition;

        for(auto& col:depth_buffer){
            for(auto& z: col){
                z = 0;
            }
        }
        Matrix camera_translation = getInverseTranslationMatrix(camera_pos);
        Matrix camera_rotation = transposeMatrix(getRotationMatrix(camera_orientation));
        Matrix camera_transform = camera_rotation * camera_translation;
        int outside_count = 0;
        for (auto& instance: instances){
            Matrix rotation = getRotationMatrix(instance.angles);
            Matrix final_transform = camera_transform * getTranslationMatrix(instance.translation)*rotation*getScalingMatrix(instance.scale);

            ObjectRelativePositionToPlane relative_position = ObjectRelativePositionToPlane::CompletelyInside;
            
            for(auto& plane: clipping_volume){
                float distance = getSignedDistance(final_transform*instance.bounding_sphere.first,plane);
                float radius = instance.scale * instance.bounding_sphere.second;
                if(distance > radius){
                    continue;
                }
                else if(distance < -radius){
                    relative_position = ObjectRelativePositionToPlane::CompletelyOutside;
                    outside_count++;
                    break;
                }
                else{
                    relative_position = ObjectRelativePositionToPlane::InBetween;
                    continue;
                    }
            }

            if(relative_position != ObjectRelativePositionToPlane::CompletelyOutside){
                for(auto& vertex: instance.model_ptr->vertices){
                    vertices_buff.push_back(final_transform*vertex);
                }

                triangle_buff1.clear();
                for(auto& triangle:instance.model_ptr->triangles){
                    TempTriangle temp_triangle = getTempTriangle(triangle,vertices_buff,rotation);
                    if(isTriangleVisible(temp_triangle)){
                        triangle_buff1.push_back(temp_triangle);
                    }
                } 
            }

            if(relative_position == ObjectRelativePositionToPlane::CompletelyInside){
                // for(auto& triangle: triangle_buff1){
                //     projected_temp_triangles_buff.push_back(projectTriangle(triangle,viewport_pos.z));
                // }

                for (auto& triangle: triangle_buff1){
                    auto intensitites = computeLighting(triangle,lights,camera_transform,camera_rotation);
                    ranges.push_back(drawTriangle(scene,depth_buffer,triangle,intensitites[0],intensitites[1],intensitites[2],viewport_pos.z));
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
                    auto intensitites = computeLighting(triangle,lights,camera_transform,camera_rotation);
                    ranges.push_back(drawTriangle(scene,depth_buffer,triangle,intensitites[0],intensitites[1],intensitites[2],viewport_pos.z));
                }
            }

            vertices_buff.clear();
            projected_triangles_buff.clear();
            projected_temp_triangles_buff.clear();
            ranges.clear();
        }
        // break

        window.draw(scene);
        window.draw(fps);

        wireframe_ranges.clear();
        scene.clear();
        
        window.display();
    }
}