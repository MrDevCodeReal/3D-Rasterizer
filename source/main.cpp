#include <iostream>
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <optional>
using std::vector;

class ObjectTransform{
    public:
        Vector3 position;
        Vector3 rotation;
        Vector3 scale;
        //basis vectors
        Vector3 forward;
        Vector3 up;
        Vector3 right;
        ObjectTransform(){
            position = {0, 0, 0};
            rotation = {0, 0, 0};
            scale = {1, 1, 1};
            forward = {0, 0, 1};
            up = {0, 1, 0};
            right = {1, 0, 0};
        }

        void ApplyRotation(){
            float pitch = rotation.x;
            float yaw = rotation.y;
            float roll = rotation.z;

            Matrix yawMat = MatrixRotateY(yaw);
            Matrix pitchMat = MatrixRotateX(pitch);
            Matrix rollMat = MatrixRotateZ(roll);
            Matrix rotationMat = MatrixMultiply(MatrixMultiply(rollMat, pitchMat), yawMat);

            forward = Vector3Transform({0, 0, 1}, rotationMat);
            up = Vector3Transform({0, 1, 0}, rotationMat);
            right = Vector3Transform({1, 0, 0}, rotationMat);
        }

        void moveInDirection(Vector3 direction, float step){
            position = Vector3Add(position, Vector3Scale(direction, step*GetFrameTime()));
        }
};

class object{
    public:
        vector<Vector3> vertices = {};
        vector<Vector3> indices = {};
        ObjectTransform transform;
        Color color;
        object(Vector3 pos, Vector3 rot, Vector3 scl, Color col){
            transform.position = pos;
            transform.rotation = rot;
            transform.scale = scl;
            color = col;
        };

        object(){
            transform.position = {0, 0, 0};
            transform.rotation = {0, 0, 0};
            transform.scale = {1, 1, 1};
        };

        void set_vertices(vector<Vector3> verts){
            vertices = verts;
        }

        void set_indices(vector<Vector3> inds){
            indices = inds;
        }
};

object Cube(Vector3 pos, Vector3 rot, Vector3 scl, Color col){
    object cube{pos, rot, scl, col};
    cube.set_vertices(
        {{-1, 1, 1},
        {1, 1, 1},
        {1, -1, 1},
        {-1, -1, 1},
        
        {1, 1, -1},
        {-1, 1, -1},
        {-1, -1, -1}, 
        {1, -1, -1}});

    cube.set_indices(
        {{2, 1, 0},
        {3, 2, 0},

        {6, 5, 4},
        {7, 6, 4},

        {7, 4, 1},
        {2, 7, 1},

        {3, 0, 5},
        {6, 3, 5},

        {1, 4, 5},
        {0, 1, 5},

        {7, 2, 3},
        {6, 7, 3}}
    );
    return cube;
}

object Plane(Vector3 pos, Vector3 rot, Vector3 scl, Color col){
    object plane(pos, rot, scl, col);
    plane.set_vertices({
        {-1, 0, 1},
        {1, 0, 1},
        {-1, 0, -1},
        {1, 0, -1}
    });

    plane.set_indices({
        {2, 1, 0},
        {1, 2, 3}
    });

    return plane;
}

class camera{
    public:
        ObjectTransform transform;
        float nearClipPlane;
        float fieldOfView;
        camera(Vector3 pos, Vector3 rot, float fov, float ncp){
            transform.position = pos;
            transform.rotation = rot;
            fieldOfView = fov;
            nearClipPlane = ncp;
        }
};

Vector3 RotateByBasisVectors(Vector3 vertex, ObjectTransform transform){
    Vector3 ihat = Vector3Scale(transform.right, vertex.x);
    Vector3 jhat = Vector3Scale(transform.up, vertex.y);
    Vector3 khat = Vector3Scale(transform.forward, vertex.z);

    float x = (ihat.x + jhat.x + khat.x);
    float y = (ihat.y + jhat.y+ khat.y);
    float z = (ihat.z + jhat.z + khat.z);

    return {x, y, z};
}

Vector3 RotateByInverseBasisVectors(Vector3 vertex, ObjectTransform transform){
    Vector3 ihat = {transform.right.x, transform.up.x, transform.forward.x};
    Vector3 jhat = {transform.right.y, transform.up.y, transform.forward.y};
    Vector3 khat = {transform.right.z, transform.up.z, transform.forward.z};

    ihat = Vector3Scale(ihat, vertex.x);
    jhat = Vector3Scale(jhat, vertex.y);
    khat = Vector3Scale(khat, vertex.z);

    float x = (ihat.x + jhat.x + khat.x);
    float y = (ihat.y + jhat.y+ khat.y);
    float z = (ihat.z + jhat.z + khat.z);

    return {x, y, z};
}

Vector3 toWorldSpace(Vector3 vertex, object obj){
    ObjectTransform trnsfrm = obj.transform;
    Matrix scaleMat = MatrixScale(trnsfrm.scale.x, trnsfrm.scale.y, trnsfrm.scale.z);
    Vector3 scaledVertex = Vector3Transform(vertex, scaleMat);

    Vector3 rotatedVertex = RotateByBasisVectors(scaledVertex, trnsfrm);

    Vector3 worldPos = Vector3Add(rotatedVertex, obj.transform.position);
    return worldPos;
}

Vector3 toViewSpace(Vector3 vertex, object obj, camera cam){
    Vector3 worldVert = toWorldSpace(vertex, obj);

    Vector3 TranslatedVertex = Vector3Subtract(worldVert, cam.transform.position);

    Vector3 viewPos = RotateByInverseBasisVectors(TranslatedVertex, cam.transform);
    return viewPos;
}

Vector2 toScreenSpace(Vector3 vertex, float fov){
    float screenHeightWorld = tanf(fov/2) * 2;
    float pixelPerUnit = GetScreenHeight() / screenHeightWorld / vertex.z;

    float sx = vertex.x * pixelPerUnit;
    float sy = vertex.y * pixelPerUnit;

    return Vector2{(GetScreenWidth()/2) + sx, (GetScreenHeight()/2) - sy};
}

Vector2 perpendicularVector(Vector2 v){
    return {v.y, -v.x};
}

bool pointOnRight(Vector2 a, Vector2 b, Vector2 c){
    Vector2 AC = Vector2Subtract(a, c);
    Vector2 ABPERP = perpendicularVector(Vector2Subtract(a, b));
    return (Vector2DotProduct(AC, ABPERP) <= 0);
}

bool pointInTriangle(Vector2 point, Vector2 triangle[]){
    bool A = pointOnRight(triangle[0], triangle[1], point);
    bool B = pointOnRight(triangle[1], triangle[2], point);
    bool C = pointOnRight(triangle[2], triangle[0], point);

    return (A && B && C);
}

void fillTriangle(Vector2 triangle[], Color (&colorBuffer)[480][640], Color triangleColor){
    int bounding_box[4];
    float screenHeight = GetScreenHeight();
    float screenWidth = GetScreenWidth();

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
                colorBuffer[py][px] = triangleColor;
            }
        }
    }
}

int main(){
    int screenHeight = 480;
	int screenWidth = 640;

	SetTraceLogLevel(LOG_NONE);
    InitWindow(screenWidth, screenHeight, "Golf Game");
	SetTargetFPS(60);
    SetWindowTitle("3D");
    DisableCursor();

    Color BACKGROUND = {25, 25, 35, 255};
    Color colorBuffer[480][640] = {};

    camera cam({0, 2, -2}, {0, 0, 0}, 90*DEG2RAD, 0.1f);

    //mesh info
    object plane = Plane({0, 0, 0}, {0, 0, 0}, {2, 2, 2}, GRAY);

    object cube = Cube({0, 2, 0}, {0, 0, 0}, {1, 1, 1}, RED);

    vector<object> objects = {plane, cube};

    Image img = GenImageColor(screenWidth, screenHeight, BACKGROUND);
    Texture2D tex = LoadTextureFromImage(img);

	while (!WindowShouldClose()){

        for (int y=0; y<=screenHeight-1; y++)
                for (int x=0; x<=screenWidth-1; x++)
                    colorBuffer[y][x] = BACKGROUND;

        if (IsKeyDown(KEY_W)){
            cam.transform.moveInDirection(cam.transform.forward, 3.0f);
        }if (IsKeyDown(KEY_S)){
            cam.transform.moveInDirection(cam.transform.forward, -3.0f);
        }if (IsKeyDown(KEY_A)){
            cam.transform.moveInDirection(cam.transform.right, -3.0f);
        }if (IsKeyDown(KEY_D)){
            cam.transform.moveInDirection(cam.transform.right, 3.0f);
        }

        cam.transform.rotation.y += GetMouseDelta().x * 0.01;
        cam.transform.rotation.x += GetMouseDelta().y * 0.01;
        cam.transform.ApplyRotation();

        for (object obj : objects){
            obj.transform.ApplyRotation();
            for (Vector3 inds: obj.indices){           
                Vector3 v1 = toViewSpace(obj.vertices[(int)inds.x], obj, cam);
                Vector3 v2 = toViewSpace(obj.vertices[(int)inds.y], obj, cam);
                Vector3 v3 = toViewSpace(obj.vertices[(int)inds.z], obj, cam);

                int insideVertices = 0;

                bool v1In = (v1.z > cam.nearClipPlane);
                bool v2In = (v2.z > cam.nearClipPlane);
                bool v3In = (v3.z > cam.nearClipPlane);

                if (v1In){
                    insideVertices++;
                }if (v2In){
                    insideVertices++;
                }if (v3In){
                    insideVertices++;
                }

                if (insideVertices == 0){
                    continue;
                }else if (insideVertices == 1){
                    Vector3 A;
                    Vector3 B;
                    Vector3 C;
                    if (v1In){
                        A = v1;
                        B = v2;
                        C = v3;
                    }else if (v2In){
                        A = v2;
                        B = v3;
                        C = v1;
                    }else{
                        A = v3;
                        B = v1;
                        C = v2;
                    }

                    float ABt = (cam.nearClipPlane - A.z) / (B.z - A.z);
                    float ACt = (cam.nearClipPlane - A.z) / (C.z - A.z);

                    Vector3 AB = Vector3Lerp(A, B, ABt);
                    Vector3 AC = Vector3Lerp(A, C, ACt);

                    Vector2 triangle[3] = {
                        toScreenSpace(AC, cam.fieldOfView),
                        toScreenSpace(AB, cam.fieldOfView),
                        toScreenSpace(A, cam.fieldOfView),
                    };

                    fillTriangle(triangle, colorBuffer, obj.color);
                }else if (insideVertices == 2){
                    Vector3 A;
                    Vector3 B;
                    Vector3 C;
                    if (!v1In){
                        A = v1;
                        B = v2;
                        C = v3;
                    }else if (!v2In){
                        A = v2;
                        B = v3;
                        C = v1;
                    }else{
                        A = v3;
                        B = v1;
                        C = v2;
                    }

                    float ACt = (cam.nearClipPlane - A.z) / (C.z - A.z);
                    float ABt = (cam.nearClipPlane - A.z) / (B.z - A.z);

                    Vector3 AC = Vector3Lerp(A, C, ACt);
                    Vector3 AB = Vector3Lerp(A, B, ABt);

                    Vector2 triangle1[3] = {
                        toScreenSpace(C, cam.fieldOfView),
                        toScreenSpace(B, cam.fieldOfView),
                        toScreenSpace(AB, cam.fieldOfView),
                    };
                    Vector2 triangle2[3] = {
                        toScreenSpace(AB, cam.fieldOfView),
                        toScreenSpace(AC, cam.fieldOfView),
                        toScreenSpace(C, cam.fieldOfView),
                    };

                    fillTriangle(triangle1, colorBuffer, obj.color);
                    fillTriangle(triangle2, colorBuffer, obj.color);
                }else{
                    Vector2 triangle[3] = {
                        toScreenSpace(v3, cam.fieldOfView),
                        toScreenSpace(v2, cam.fieldOfView),
                        toScreenSpace(v1, cam.fieldOfView),
                    };
                    fillTriangle(triangle, colorBuffer, obj.color);
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