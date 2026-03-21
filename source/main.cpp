#include <iostream>
#include "raylib.h"
#include "raymath.h"

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

	//SetTraceLogLevel(LOG_NONE);
    InitWindow(screenWidth, screenHeight, "Golf Game");
	SetTargetFPS(60);
    SetWindowTitle("3D");

    Color colorBuffer[480][640] = {};

    //triangle info
    Vector2 triangle_points[3] = {{213, 320}, {426, 320}, {320, 160}};

    //bounding box to ensure speed
    int bounding_box[4] = {9999, -9999, 9999, -9999};

    Image img = GenImageColor(screenWidth, screenHeight, BLACK);
    Texture2D tex = LoadTextureFromImage(img);

	while (!WindowShouldClose()){

        bounding_box[0] = 9999;
        bounding_box[1] = -9999;
        bounding_box[2] = 9999;
        bounding_box[3] = -9999;
        for (int i=0; i<3; i++){
            if (triangle_points[i].y < bounding_box[0]){
                bounding_box[0] = triangle_points[i].y;
            }
            if (triangle_points[i].y > bounding_box[1]){
                bounding_box[1] = triangle_points[i].y;
            }
            if (triangle_points[i].x < bounding_box[2]){
                bounding_box[2] = triangle_points[i].x;
            }
            if (triangle_points[i].x > bounding_box[3]){
                bounding_box[3] = triangle_points[i].x;
            }
        }

        for (int y=0; y<screenHeight; y++)
            for (int x=0; x<screenWidth; x++)
                colorBuffer[y][x] = {25, 25, 45};

        for (int py=bounding_box[0]; py<=bounding_box[1]; py++){
            for (int px=bounding_box[2]; px<=bounding_box[3]; px++){
                if (pointInTriangle({(float)px, (float)py}, triangle_points)){
                    colorBuffer[py][px] = GREEN;
                }
            }
        }

        UpdateTexture(tex, colorBuffer);

		BeginDrawing();
        DrawTexture(tex, 0, 0, WHITE);
		
        DrawCircle(triangle_points[0].x, triangle_points[0].y, 5, RED);
        DrawCircle(triangle_points[1].x, triangle_points[1].y, 5, RED);
        DrawCircle(triangle_points[2].x, triangle_points[2].y, 5, RED);

		EndDrawing();
	}
	
    UnloadTexture(tex);
	CloseWindow();

    return 0;
}