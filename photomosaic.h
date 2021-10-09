#include <stdio.h>
#define TILE_MAX_LINE_SIZE 512

typedef struct
{
    int red, green, blue;
} Pixel;

typedef struct
{
    Pixel **pixels;
    char type[3];
    int width, height, maxValue;
    double avgG, avgB, avgR;
} Tile;

float calculateDistance(Tile *tile, Tile *piece);

void calculateAvgColor(Tile *tile);

void readTiles(char *dirName, struct dirent **filesPath, int *filesCount, Tile *tiles);

Tile *readImage(FILE *image);

int **matchTiles(Tile **imagePieces, Tile *tiles, int lines, int columns, int tilesQtd);

Tile **splitInputImage(Tile *image, int tileHeight, int tileWidth);

void writeFile(FILE *outputFile, Tile *originalImage, int **tileIndexes, Tile *tiles, int lines, int columns);
// void writeFile(Tile *originalImage, Tile **imagePieces, int lines, int columns);
// void writeFile(Tile *originalImage, int lines, int columns);