#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <unistd.h>
#include "photomosaic.h"

void freeDirentArray(struct dirent **files, int count)
{
    for (int i = 0; i < count; i++)
    {
        free(files[i]);
    }
    free(files);
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
    FILE *inputImageFile = stdin;
    FILE *outputImageFile = stdout;
    // FILE *inputImageFile;
    // FILE *outputImageFile;

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

    if (!strlen(directoryName) || (!strlen(inputFileName) && isatty(0)) || (!strlen(outputFileName) && isatty(1)))
    {
        fprintf(stderr, "Missing arguments.\n");
        fprintf(stderr, "Execute ./mosaico -h for help.\n");
        exit(1);
    }

    /* Scaneia o diretório de tiles, conta a quantidade de arquivos e 
        armazena seus dados em um array */
    fprintf(stderr, "Reading tiles from %s\n", directoryName);
    filesCount = scandir(directoryName, &filesPath, filterFileType, alphasort);
    tilesCount = filesCount;

    if (filesCount == -1)
    {
        fprintf(stderr, "The tile directory does not exist.\n");
        exit(1);
    }

    if (isatty(0))
    {
        inputImageFile = fopen(inputFileName, "r");

        if (!inputImageFile)
        {
            freeDirentArray(filesPath, filesCount);
            fprintf(stderr, "Error reading input image.\n");
            exit(1);
        }
    }

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

    inputImage = readImage(inputImageFile);

    fprintf(stderr, "Input image is PPM %s, %dx%d pixels\n", inputImage->type, inputImage->width, inputImage->height);

    if (isatty(0))
        fclose(inputImageFile);

    //Get the amount of tiles in the main image
    int imagePiecesLines = ceil((double)inputImage->height / tiles[0].height);
    int imagePiecesColumns = ceil((double)inputImage->width / tiles[0].width);

    fprintf(stderr, "Spliting input image.\n");
    Tile **imagePieces = splitInputImage(inputImage, tiles[0].height, tiles[0].width);

    fprintf(stderr, "Matching tiles.\n");
    int **matchedTiles = matchTiles(imagePieces, tiles, imagePiecesLines, imagePiecesColumns, tilesCount);

    fprintf(stderr, "Writing output file.\n");

    if (isatty(1))
    {
        outputImageFile = fopen(outputFileName, "w");

        if (!outputImageFile)
        {
            fprintf(stderr, "Error reading output file!\n");
            exit(1);
        }
    }

    writeFile(outputImageFile, inputImage, matchedTiles, tiles, imagePiecesLines, imagePiecesColumns);

    if (isatty(1))
        fclose(outputImageFile);

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
    freeDirentArray(filesPath, filesCount);

    return 0;
}