#include <iostream>
#include "raylib.h"
#include "raymath.h"
#include <vector>
using std::vector;

class object{
    public:
        vector<Vector3> vertices = {};
        vector<Vector3> indices = {};
        Vector3 position;
        Vector3 rotation;
        Vector3 scale;
        object(Vector3 pos, Vector3 rot, Vector3 scl){
            position = pos;
            rotation = rot;
            scale = scl;
        };

        void set_vertices(vector<Vector3> verts){
            vertices = verts;
        }

        void set_indices(vector<Vector3> inds){
            indices = inds;
        }
};

class camera{
    public:
        Vector3 position;
        Vector3 rotation;
        camera(Vector3 pos, Vector3 rot){
            position = pos;
            rotation = rot;
        }
};

Vector3 toWorldSpace(Vector3 vertex, object obj){
    Matrix world = MatrixMultiply(
        MatrixScale(obj.scale.x, obj.scale.y, obj.scale.z), MatrixMultiply(
            MatrixRotateXYZ(Vector3Scale(obj.rotation, -DEG2RAD)), 
            MatrixTranslate(obj.position.x, obj.position.y, obj.position.z)
        ));

    Vector3 worldPos = Vector3Transform(vertex, world);
    return worldPos;
}

Vector3 toViewSpace(Vector3 vertex, object obj, camera cam){
    Vector3 worldVert = toWorldSpace(vertex, obj);
    Matrix view = MatrixMultiply(MatrixInvert(MatrixRotateXYZ(Vector3Scale(cam.rotation, -DEG2RAD))), MatrixTranslate(-cam.position.x, -cam.position.y, -cam.position.z));

    Vector3 viewPos = Vector3Transform(worldVert, view);
    return viewPos;
}

Vector2 toScreenSpace(Vector3 vertex, object obj, camera cam, float fov){
    Vector3 viewVert = toViewSpace(vertex, obj, cam);

    if (viewVert.z <= 0){
        return {-1, -1};
    }

    float screenHeightWorld = tanf(fov/2) * 2;
    float pixelPerUnit = GetScreenHeight() / screenHeightWorld / viewVert.z;

    float sx = viewVert.x * pixelPerUnit;
    float sy = viewVert.y * pixelPerUnit;

    return {(GetScreenWidth()/2) + sx, (GetScreenHeight()/2) - sy};
}

Vector2 perpendicularVector(Vector2 v){
    return {v.y, -v.x};
}

bool pointOnRight(Vector2 a, Vector2 b, Vector2 c){
    Vector2 AC = Vector2Subtract(a, c);
    Vector2 ABPERP = perpendicularVector(Vector2Subtract(a, b));
    return (Vector2DotProduct(AC, ABPERP) >= 0);
}

bool pointInTriangle(Vector2 point, Vector2 triangle[]){
    bool A = pointOnRight(triangle[0], triangle[1], point);
    bool B = pointOnRight(triangle[1], triangle[2], point);
    bool C = pointOnRight(triangle[2], triangle[0], point);

    return (A && B && C);
}

int main(){
    int screenHeight = 480;
	int screenWidth = 640;

	SetTraceLogLevel(LOG_NONE);
    InitWindow(screenWidth, screenHeight, "Golf Game");
	SetTargetFPS(60);
    SetWindowTitle("3D");

    Color BACKGROUND = {25, 25, 35, 255};
    Color colorBuffer[480][640] = {};

    camera cam({0, 0, 0}, {0, 0, 0});

    //mesh info
    object plane({0, -0.5, 2}, {45, 45, 0}, {1, 1, 1});
    plane.set_vertices({
        {-1, 0, 1},
        {1, 0, 1},
        {-1, 0, 0},
        {1, 0, 0}
    });
    plane.set_indices({
        {2, 1, 0},
        {1, 2, 3}
    });

    vector<object> objects = {plane};

    int bounding_box[4];

    Image img = GenImageColor(screenWidth, screenHeight, BACKGROUND);
    Texture2D tex = LoadTextureFromImage(img);

	while (!WindowShouldClose()){

        for (int y=0; y<=screenHeight-1; y++)
                for (int x=0; x<=screenWidth-1; x++)
                    colorBuffer[y][x] = BACKGROUND;

        if (IsKeyDown(KEY_W)){
            cam.position.z += 3 * GetFrameTime();
        }if (IsKeyDown(KEY_S)){
            cam.position.z -= 3 * GetFrameTime();
        }if (IsKeyDown(KEY_A)){
            cam.position.x -= 3 * GetFrameTime();
        }if (IsKeyDown(KEY_D)){
            cam.position.x += 3 * GetFrameTime();
        }

        cam.rotation.y -= GetMouseDelta().x;
        cam.rotation.x += GetMouseDelta().y;

        for (object obj : objects){
            for (Vector3 inds: obj.indices){
                Vector2 triangle[3] = {};
                
                triangle[0] = toScreenSpace(obj.vertices[(int)inds.x], obj, cam, 90*DEG2RAD);
                triangle[1] = toScreenSpace(obj.vertices[(int)inds.y], obj, cam, 90*DEG2RAD);
                triangle[2] = toScreenSpace(obj.vertices[(int)inds.z], obj, cam, 90*DEG2RAD);

                bounding_box[0] = fmin(fmin(triangle[0].y, triangle[1].y), triangle[2].y);
                bounding_box[1] = fmax(fmax(triangle[0].y, triangle[1].y), triangle[2].y);
                bounding_box[2] = fmin(fmin(triangle[0].x, triangle[1].x), triangle[2].x);
                bounding_box[3] = fmax(fmax(triangle[0].x, triangle[1].x), triangle[2].x);

                bounding_box[0] = Clamp(bounding_box[0], 0, screenHeight-1);
                bounding_box[1] = Clamp(bounding_box[1], 0, screenHeight-1);
                bounding_box[2] = Clamp(bounding_box[2], 0, screenWidth-1);
                bounding_box[3] = Clamp(bounding_box[3], 0, screenWidth-1);

                for (int py=bounding_box[0]; py<=bounding_box[1]; py++){
                    for (int px=bounding_box[2]; px<=bounding_box[3]; px++){
                        if (pointInTriangle({(float)px, (float)py}, triangle)){
                            colorBuffer[py][px] = GREEN;
                        }
                    }
                }
            }
        }

        UpdateTexture(tex, colorBuffer);

		BeginDrawing();
        DrawTexture(tex, 0, 0, WHITE);
		EndDrawing();
	}
	
    UnloadTexture(tex);
    UnloadImage(img);
	CloseWindow();

    return 0;
}