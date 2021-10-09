#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include "photomosaic.h"

#define DIR_NAME "./tiles20"

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

int filterFileType(const struct dirent *currentDir)
{
    if (currentDir->d_type != DT_REG)
    {
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    FILE *inputImageFile;
    Tile *inputImage = NULL;
    Tile *tiles = NULL;
    struct dirent **filesPath = NULL;
    int filesCount = 0;
    int tilesCount = 0;
    char directoryName[200] = "", inputFileName[200] = "", outputFileName[200] = "";

    int option;
    while ((option = getopt(argc, argv, "p:i:o:h::")) != -1)
    {
        switch (option)
        {
        case 'p':
            strcpy(directoryName, optarg);
            break;
        case 'i':
            strcpy(inputFileName, optarg);
            break;
        case 'o':
            strcpy(outputFileName, optarg);
            break;
        case 'h':
            printf("help\n");
            exit(1);
            break;
        case '?':
            if (optopt == 'i' || optopt == 'o' || optopt == 'p')
                fprintf(stderr, "Opção -%c requer um argumento.\n", optopt);
            else
            {
                fprintf(stderr, "Opção desconhecida.\nExecute ./mosaico -h para obter ajuda.\n");
            }
            return 1;
        default:
            abort();
            break;
        }
    }

    if (!strlen(directoryName) || !strlen(inputFileName) || !strlen(outputFileName))
    {
        fprintf(stderr, "Missing arguments.\n");
        fprintf(stderr, "Execute ./mosaico -h for help.\n");
        exit(1);
    }

    /* Scaneia o diretório de tiles, conta a quantidade de arquivos e 
        armazena seus dados em um array */
    fprintf(stderr, "Reading tiles from %s\n", DIR_NAME);
    filesCount = scandir(directoryName, &filesPath, filterFileType, alphasort);
    tilesCount = filesCount;

    //Aloca espaço para o vetor de tiles
    tiles = calloc(tilesCount, sizeof(Tile));

    if (tiles == NULL)
    {
        fprintf(stderr, "Error allocationg tiles array!");
        exit(1);
    }

    readTiles(directoryName, filesPath, &tilesCount, tiles);

    fprintf(stderr, "Tile size is %dx%d\n", tiles[0].width, tiles[0].height);
    fprintf(stderr, "%d tiles read.\n", tilesCount);

    fprintf(stderr, "Reading input file\n");
    inputImageFile = fopen(inputFileName, "r");

    if (!inputImageFile)
    {
        fprintf(stderr, "Error reading input image.");
        exit(1);
    }

    inputImage = readImage(inputImageFile);

    fprintf(stderr, "Input image is PPM %s, %dx%d pixels\n", inputImage->type, inputImage->width, inputImage->height);
    fclose(inputImageFile);

    //Get the amount of tiles in the main image
    int imagePiecesLines = ceil((double)inputImage->height / tiles[0].height);
    int imagePiecesColumns = ceil((double)inputImage->width / tiles[0].width);

    fprintf(stderr, "Spliting input image.\n");
    Tile **imagePieces = splitInputImage(inputImage, tiles[0].height, tiles[0].width);

    fprintf(stderr, "Matching tiles.\n");
    int **matchedTiles = matchTiles(imagePieces, tiles, imagePiecesLines, imagePiecesColumns, tilesCount);

    fprintf(stderr, "Writing output file.\n");
    writeFile(outputFileName, inputImage, matchedTiles, tiles, imagePiecesLines, imagePiecesColumns);

    fprintf(stderr, "Freeing allocated memory.\n");

    //Free imagePieces and matched pieces
    for (int i = 0; i < imagePiecesLines; i++)
    {
        free(matchedTiles[i]);
        for (int j = 0; j < imagePiecesColumns; j++)
        {
            for (int k = 0; k < imagePieces[i][j].height; k++)
                free(imagePieces[i][j].pixels[k]);
            free(imagePieces[i][j].pixels);
        }
        free(imagePieces[i]);
    }
    free(matchedTiles);
    free(imagePieces);

    //free tiles array
    for (int i = 0; i < tilesCount; i++)
    {
        for (int j = 0; j < tiles[i].height; j++)
            free(tiles[i].pixels[j]);
        free(tiles[i].pixels);
    }
    free(tiles);

    //Free input image
    for (int i = 0; i < inputImage->height; i++)
        free(inputImage->pixels[i]);
    free(inputImage->pixels);
    free(inputImage);

    //Free filesPath
    for (int i = 0; i < filesCount; i++)
    {
        free(filesPath[i]);
    }
    free(filesPath);

    return 0;
}