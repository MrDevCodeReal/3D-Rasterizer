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

bool pointInTriangle(Vector2 point, Vector2 triangle[], int indices[]){
    bool A = pointOnRight(triangle[indices[0]], triangle[indices[1]], point);
    bool B = pointOnRight(triangle[indices[1]], triangle[indices[2]], point);
    bool C = pointOnRight(triangle[indices[2]], triangle[indices[0]], point);

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
    Vector2 triangle_points[3] = {{0, 0},  {0, 480}, {640, 480}};
    int triangle_indices[3] = {0, 1, 2};

    //bounding box to ensure speed
    int bounding_box[4];

    Image img = GenImageColor(screenWidth, screenHeight, BLACK);
    Texture2D tex = LoadTextureFromImage(img);

	while (!WindowShouldClose()){

        bounding_box[0] = 999;
        bounding_box[1] = -999;
        bounding_box[2] = 999;
        bounding_box[3] = -999;
        for (int i=0; i<3; i++){
            if (triangle_points[triangle_indices[i]].y < bounding_box[0]){
                bounding_box[0] = triangle_points[triangle_indices[i]].y;
            }
            if (triangle_points[triangle_indices[i]].y > bounding_box[1]){
                bounding_box[1] = triangle_points[triangle_indices[i]].y;
            }
            if (triangle_points[triangle_indices[i]].x < bounding_box[2]){
                bounding_box[2] = triangle_points[triangle_indices[i]].x;
            }
            if (triangle_points[triangle_indices[i]].x > bounding_box[3]){
                bounding_box[3] = triangle_points[triangle_indices[i]].x;
            }
        }

        for (int y=0; y<=screenHeight-1; y++)
            for (int x=0; x<=screenWidth-1; x++)
                colorBuffer[y][x] = {255, 255, 255};

        for (int py=bounding_box[0]; py<bounding_box[1]; py++){
            for (int px=bounding_box[2]; px<bounding_box[3]; px++){
                if (pointInTriangle({(float)px, (float)py}, triangle_points, triangle_indices)){
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
    UnloadImage(img);
	CloseWindow();

    return 0;
}