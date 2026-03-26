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

        void Rotate(){
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
};

class object{
    public:
        vector<Vector3> vertices = {};
        vector<Vector3> indices = {};
        ObjectTransform transform;
        object(Vector3 pos, Vector3 rot, Vector3 scl){
            transform.position = pos;
            transform.rotation = rot;
            transform.scale = scl;
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
        ObjectTransform transform;
        float fieldOfView;
        camera(Vector3 pos, Vector3 rot, float fov){
            transform.position = pos;
            transform.rotation = rot;
            fieldOfView = fov;
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

    Vector3 worldPos = Vector3Transform(rotatedVertex, MatrixTranslate(trnsfrm.position.x, trnsfrm.position.y, trnsfrm.position.z));
    return worldPos;
}

Vector3 toViewSpace(Vector3 vertex, object obj, camera cam){
    Vector3 worldVert = toWorldSpace(vertex, obj);

    Vector3 inverseCamPos = Vector3Scale(cam.transform.position, -1);
    Vector3 TranslatedVertex = Vector3Transform(worldVert, MatrixTranslate(inverseCamPos.x, inverseCamPos.y, inverseCamPos.z));

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

void fillTriangle(Vector2 triangle[], Color (&colorBuffer)[480][640]){
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
                colorBuffer[py][px] = GREEN;
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

    camera cam({0, 0, 0}, {0, 0, 0}, 90*DEG2RAD);

    //mesh info
    object plane({0, -0.5, 2}, {0, 0, 0}, {1, 1, 1});
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

    Image img = GenImageColor(screenWidth, screenHeight, BACKGROUND);
    Texture2D tex = LoadTextureFromImage(img);

	while (!WindowShouldClose()){

        for (int y=0; y<=screenHeight-1; y++)
                for (int x=0; x<=screenWidth-1; x++)
                    colorBuffer[y][x] = BACKGROUND;

        if (IsKeyDown(KEY_W)){
            cam.transform.position.z += 3 * GetFrameTime();
        }if (IsKeyDown(KEY_S)){
            cam.transform.position.z -= 3 * GetFrameTime();
        }if (IsKeyDown(KEY_A)){
            cam.transform.position.x -= 3 * GetFrameTime();
        }if (IsKeyDown(KEY_D)){
            cam.transform.position.x += 3 * GetFrameTime();
        }

        cam.transform.rotation.y += GetMouseDelta().x * 0.01;
        cam.transform.rotation.x += GetMouseDelta().y * 0.01;
        cam.transform.Rotate();

        for (object obj : objects){
            obj.transform.Rotate();
            for (Vector3 inds: obj.indices){           
                Vector3 v1 = toViewSpace(obj.vertices[(int)inds.x], obj, cam);
                Vector3 v2 = toViewSpace(obj.vertices[(int)inds.y], obj, cam);
                Vector3 v3 = toViewSpace(obj.vertices[(int)inds.z], obj, cam);

                int insideVertices = 0;

                bool v1In = (v1.z > 0);
                bool v2In = (v2.z > 0);
                bool v3In = (v3.z > 0);

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

                    float ABt = -A.z / (B.z - A.z);
                    float ACt = -A.z / (C.z - A.z);

                    Vector3 AB = Vector3Lerp(A, B, ABt);
                    Vector3 AC = Vector3Lerp(A, C, ACt);

                    Vector2 triangle[3] = {
                        toScreenSpace(AC, cam.fieldOfView),
                        toScreenSpace(AB, cam.fieldOfView),
                        toScreenSpace(A, cam.fieldOfView),
                    };

                    fillTriangle(triangle, colorBuffer);
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

                    float CAt = -A.z / (C.z - A.z);
                    float BAt = -A.z / (B.z - A.z);

                    Vector3 AC = Vector3Lerp(A, C, CAt);
                    Vector3 AB = Vector3Lerp(A, B, BAt);

                    Vector2 triangle1[3] = {
                        toScreenSpace(B, cam.fieldOfView),
                        toScreenSpace(C, cam.fieldOfView),
                        toScreenSpace(AB, cam.fieldOfView),
                    };
                    Vector2 triangle2[3] = {
                        toScreenSpace(C, cam.fieldOfView),
                        toScreenSpace(AB, cam.fieldOfView),
                        toScreenSpace(AC, cam.fieldOfView),
                    };

                    fillTriangle(triangle1, colorBuffer);
                    fillTriangle(triangle2, colorBuffer);
                }else{
                    Vector2 triangle[3] = {
                        toScreenSpace(v3, cam.fieldOfView),
                        toScreenSpace(v2, cam.fieldOfView),
                        toScreenSpace(v1, cam.fieldOfView),
                    };
                    fillTriangle(triangle, colorBuffer);
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