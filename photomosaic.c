#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <math.h>
#include "photomosaic.h"

//Exclude comments and blank lines from the file being read
int excludeInvalidLines(char line[TILE_MAX_LINE_SIZE])
{
    if (line[0] == '\0' || line[0] == '\n' || line[0] == '#')
        return 1;
    return 0;
}

void calculateAvgColor(Tile *tile)
{
    int pixelsQtd = tile->width * tile->height;
    int totalRed = 0, totalGreen = 0, totalBlue = 0;
    for (int i = 0; i < tile->height; i++)
    {
        for (int j = 0; j < tile->width; j++)
        {
            totalRed += tile->pixels[i][j].red;
            totalGreen += tile->pixels[i][j].green;
            totalBlue += tile->pixels[i][j].blue;
        }
    }
    tile->avgR = totalRed / pixelsQtd;
    tile->avgG = totalGreen / pixelsQtd;
    tile->avgB = totalBlue / pixelsQtd;
}

float calculateDistance(Tile *tile, Tile *piece)
{
    float distance;
    float redMean = (tile->avgR + piece->avgR) / 2;

    distance = abs(sqrt(
        (2 + (redMean / 256)) * pow(piece->avgR - tile->avgR, 2) +
        4 * pow(piece->avgG - tile->avgG, 2) +
        (2 + ((255 - redMean) / 256)) * pow(piece->avgB - tile->avgB, 2)));

    return distance;
}

void readTiles(char *dirName, struct dirent **filesPath, int *filesCount, Tile *tiles)
{
    FILE *file;
    char tempPath[200];
    int errorsCount = 0;

    for (int i = 0; i < *filesCount; i++)
    {
        tempPath[0] = '\0';
        strcat(tempPath, dirName);
        strcat(tempPath, "/");
        strcat(tempPath, filesPath[i]->d_name);

        file = fopen(tempPath, "r");

        if (!file)
        {
            fprintf(stderr, "ERROR OPENING FILE: %s\n", tempPath);
            errorsCount++;
        }
        else
        {
            Tile *tmpTile = readImage(file);
            tiles[i] = *tmpTile;

            free(tmpTile);

            fclose(file);
        }
    }
    *filesCount -= errorsCount;
}

Tile *readImage(FILE *image)
{
    char line[TILE_MAX_LINE_SIZE + 1];

    Tile *newTile = malloc(sizeof(Tile));

    if (!newTile)
    {
        fprintf(stderr, "Error allocating new tile memory.\n");
        exit(1);
    }

    //get the file type (P3 or P6)
    fgets(line, TILE_MAX_LINE_SIZE, image);
    while (excludeInvalidLines(line))
        fgets(line, TILE_MAX_LINE_SIZE, image);

    strncpy(newTile->type, line, 2);
    newTile->type[2] = '\0';

    //get the file size
    fgets(line, TILE_MAX_LINE_SIZE, image);
    while (excludeInvalidLines(line))
        fgets(line, TILE_MAX_LINE_SIZE, image);

    sscanf(line, "%d %d", &newTile->width, &newTile->height);

    //get the pixel maxValue
    fgets(line, TILE_MAX_LINE_SIZE, image);
    while (excludeInvalidLines(line))
        fgets(line, TILE_MAX_LINE_SIZE, image);

    sscanf(line, "%d", &newTile->maxValue);

    //Reads each pixel of a P3 file and stores in a 2d array
    if (!strcmp(newTile->type, "P3"))
    {
        newTile->pixels = calloc(newTile->height, sizeof(Pixel *));
        for (int i = 0; i < newTile->height; i++)
        {
            newTile->pixels[i] = calloc(newTile->width, sizeof(Pixel));
            if (!newTile->pixels[i])
            {
                fprintf(stderr, "Error allocating memory.\n");
                exit(1);
            }
            for (int j = 0; j < newTile->width; j++)
            {
                fscanf(image, "%d", &newTile->pixels[i][j].red);
                fscanf(image, "%d", &newTile->pixels[i][j].green);
                fscanf(image, "%d", &newTile->pixels[i][j].blue);
            }
        }
    }
    //Reads each pixel of a P6 file and stores in a 2d array
    else if (!strcmp(newTile->type, "P6"))
    {
        newTile->pixels = calloc(newTile->height, sizeof(Pixel *));

        for (int i = 0; i < newTile->height; i++)
        {
            newTile->pixels[i] = calloc(newTile->width, sizeof(Pixel));
            if (!newTile->pixels[i])
            {
                fprintf(stderr, "Error allocating memory.\n");
                exit(1);
            }
            for (int j = 0; j < newTile->width; j++)
            {
                fread(&newTile->pixels[i][j].red, 1, 1, image);
                fread(&newTile->pixels[i][j].green, 1, 1, image);
                fread(&newTile->pixels[i][j].blue, 1, 1, image);
            }
        }
    }
    else
    {
        fprintf(stderr, "Invalid tile format: %s\n", newTile->type);
        fprintf(stderr, "Aborting\n");
        exit(1);
    }

    //Calculate the avg RGB of the newly read image
    calculateAvgColor(newTile);

    return newTile;
}

//Splits the input image into small pieces and returns a 2d array of these pieces
Tile **splitInputImage(Tile *image, int tileHeight, int tileWidth)
{
    int imagePiecesHeight = ceil((double)image->height / tileHeight);
    int imagePiecesWidth = ceil((double)image->width / tileWidth);
    int pieceCounter = 0;

    Tile **imagePieces;
    imagePieces = calloc(imagePiecesHeight, sizeof(Tile *));
    if (!imagePieces)
    {
        fprintf(stderr, "Error allocating memory.\n");
        exit(1);
    }
    for (int i = 0; i < imagePiecesHeight; i++)
    {
        imagePieces[i] = calloc(imagePiecesWidth, sizeof(Tile));
        if (!imagePieces[i])
        {
            fprintf(stderr, "Error allocating memory.\n");
            exit(1);
        }
    }

    Pixel auxPixelMatrix[500][500];

    for (int posY = 0; posY < imagePiecesHeight; posY++)
    {
        for (int posX = 0; posX < imagePiecesWidth; posX++)
        {
            imagePieces[posY][posX].maxValue = image->maxValue;
            strcpy(imagePieces[posY][posX].type, image->type);
            int tileY;
            for (tileY = 0; tileY < tileHeight && (posY * tileHeight + tileY) < image->height; tileY++)
            {
                int tileX;
                for (tileX = 0; tileX < tileWidth && (posX * tileWidth + tileX) < image->width; tileX++)
                {

                    auxPixelMatrix[tileY][tileX] = image->pixels[(posY * tileHeight) + tileY][(posX * tileWidth) + tileX];
                }
                imagePieces[posY][posX].width = tileX;
            }
            imagePieces[posY][posX].height = tileY;
            imagePieces[posY][posX].pixels = calloc(imagePieces[posY][posX].height, sizeof(Pixel *));
            if (!imagePieces[posY][posX].pixels)
            {
                fprintf(stderr, "Error allocating memory");
                exit(1);
            }
            for (int i = 0; i < imagePieces[posY][posX].height; i++)
            {
                imagePieces[posY][posX].pixels[i] = calloc(imagePieces[posY][posX].width, sizeof(Pixel));
                if (!imagePieces[posY][posX].pixels[i])
                {
                    fprintf(stderr, "Error allocating memory");
                    exit(1);
                }
                for (int j = 0; j < imagePieces[posY][posX].width; j++)
                {
                    imagePieces[posY][posX].pixels[i][j].red = auxPixelMatrix[i][j].red;
                    imagePieces[posY][posX].pixels[i][j].green = auxPixelMatrix[i][j].green;
                    imagePieces[posY][posX].pixels[i][j].blue = auxPixelMatrix[i][j].blue;
                }
            }
            calculateAvgColor(&imagePieces[posY][posX]);

            pieceCounter++;
        }
    }

    return imagePieces;
}

//Match each image pieces to individual tiles in the tiles array
//Returns an matrix of integer representing the index of each tile in the array
int **matchTiles(Tile **imagePieces, Tile *tiles, int lines, int columns, int tilesQtd)
{
    int **tileIndexes;
    tileIndexes = calloc(lines, sizeof(int *));
    if (!tileIndexes)
    {
        fprintf(stderr, "Error allocating memory.\n");
        exit(1);
    }

    for (int i = 0; i < lines; i++)
    {
        tileIndexes[i] = calloc(columns, sizeof(int));
        if (!tileIndexes[i])
        {
            fprintf(stderr, "Error allocating memory.\n");
            exit(1);
        }
    }

    int curIndex = 0;
    for (int i = 0; i < lines; i++)
    {
        for (int j = 0; j < columns; j++)
        {
            int minIndex = 0;
            double minDistance = INFINITY;
            for (int k = 0; k < tilesQtd; k++)
            {
                int tpmDistance = calculateDistance(&tiles[k], &imagePieces[i][j]);
                if (tpmDistance < minDistance)
                {
                    minDistance = tpmDistance;
                    minIndex = k;
                }
            }
            tileIndexes[i][j] = minIndex;
            curIndex++;
        }
    }

    fprintf(stderr, "Matched %d image pieces to a individual tile.\n", curIndex);
    return tileIndexes;
}

//Write the final result into a file
void writeFile(FILE *outputFile, Tile *originalImage, int **tileIndexes, Tile *tiles, int lines, int columns)
{
    //Writes the header information
    fprintf(outputFile, "%s\n", originalImage->type);
    fprintf(outputFile, "%d %d\n", originalImage->width, originalImage->height);
    fprintf(outputFile, "%d\n", originalImage->maxValue);

    //Writes each pixel from the tiles matrix
    if (!strcmp(originalImage->type, "P3"))
    {
        fprintf(stderr, "WRITING P3");
        int i = 0, j = 0, k = 0, l = 0;
        for (i = 0; i < lines; i++)
        {
            int maxK;
            if (i == (lines - 1))
                maxK = originalImage->height % tiles[tileIndexes[i][j]].height;
            else
                maxK = tiles[tileIndexes[i][j]].height;
            for (k = 0; k < maxK; k++)
            {
                for (j = 0; j < columns; j++)
                {
                    int maxL;
                    if (j == (columns - 1))
                        maxL = originalImage->width % tiles[tileIndexes[i][j]].width;
                    else
                        maxL = tiles[tileIndexes[i][j]].width;
                    for (l = 0; l < maxL; l++)
                    {
                        fprintf(outputFile, "%d %d %d ", tiles[tileIndexes[i][j]].pixels[k][l].red, tiles[tileIndexes[i][j]].pixels[k][l].green, tiles[tileIndexes[i][j]].pixels[k][l].blue);
                    }
                    l = 0;
                    fgetc(outputFile);
                    fprintf(outputFile, "\n");
                }
                j = 0;
            }
            k = 0;
        }
    }
    else if (!strcmp(originalImage->type, "P6"))
    {
        int i = 0, j = 0, k = 0, l = 0;
        for (i = 0; i < lines; i++)
        {
            int maxK;
            if (i == (lines - 1))
                maxK = originalImage->height % tiles[tileIndexes[i][j]].height;
            else
                maxK = tiles[tileIndexes[i][j]].height;
            for (k = 0; k < maxK; k++)
            {
                for (j = 0; j < columns; j++)
                {
                    int maxL;
                    if (j == (columns - 1))
                        maxL = originalImage->width % tiles[tileIndexes[i][j]].width;
                    else
                        maxL = tiles[tileIndexes[i][j]].width;
                    for (l = 0; l < maxL; l++)
                    {
                        fwrite(&tiles[tileIndexes[i][j]].pixels[k][l].red, 1, 1, outputFile);
                        fwrite(&tiles[tileIndexes[i][j]].pixels[k][l].green, 1, 1, outputFile);
                        fwrite(&tiles[tileIndexes[i][j]].pixels[k][l].blue, 1, 1, outputFile);
                    }
                    l = 0;
                }
                j = 0;
            }
            k = 0;
        }
    }
    else
    {
        fprintf(stderr, "Invalid file format!\n");
        exit(1);
    }
}