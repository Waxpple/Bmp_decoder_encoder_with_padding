//
//  main.c
//  read_image
//
//  Created by waxpple on 2020/12/1.
//

#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <OpenCL/opencl.h>
#include <memory.h>
#define DATA_OFFSET_OFFSET 0x000A
#define WIDTH_OFFSET 0x0012
#define HEIGHT_OFFSET 0x0016
#define BITS_PER_PIXEL_OFFSET 0x001C
#define HEADER_SIZE 14
#define INFO_HEADER_SIZE 40
#define NO_COMPRESION 0
#define MAX_NUMBER_OF_COLORS 0
#define ALL_COLORS_REQUIRED 0

typedef unsigned int int32;
typedef short int16;
typedef unsigned char byte;
 
void ReadImage(const char *fileName,byte **pixels, int32 *width, int32 *height, int32 *bytesPerPixel)
{
        FILE *imageFile = fopen(fileName, "rb");
        int32 dataOffset;
        fseek(imageFile, DATA_OFFSET_OFFSET, SEEK_SET);
        fread(&dataOffset, 4, 1, imageFile);
        fseek(imageFile, WIDTH_OFFSET, SEEK_SET);
        fread(width, 4, 1, imageFile);
        fseek(imageFile, HEIGHT_OFFSET, SEEK_SET);
        fread(height, 4, 1, imageFile);
        int16 bitsPerPixel;
        fseek(imageFile, BITS_PER_PIXEL_OFFSET, SEEK_SET);
        fread(&bitsPerPixel, 2, 1, imageFile);
        *bytesPerPixel = ((int32)bitsPerPixel) / 8;
 
        int paddedRowSize = (int)(4 * ceil((float)(*width*(*bytesPerPixel)) / 4.0f));
        int unpaddedRowSize = (*width)*(*bytesPerPixel);
        int totalSize = unpaddedRowSize*(*height);
        *pixels = (byte*)malloc(totalSize);
        int i = 0;
        byte *currentRowPointer = *pixels+((*height-1)*unpaddedRowSize);
        for (i = 0; i < *height; i++)
        {
            fseek(imageFile, dataOffset+(i*paddedRowSize), SEEK_SET);
            fread(currentRowPointer, 1, unpaddedRowSize, imageFile);
            currentRowPointer -= unpaddedRowSize;
        }
 
        fclose(imageFile);
}
void WriteImage(const char *fileName, byte *pixels, int32 width, int32 height,int32 bytesPerPixel)
{
        FILE *outputFile = fopen(fileName, "wb");
        //*****HEADER************//
        const char *BM = "BM";
        fwrite(&BM[0], 1, 1, outputFile);
        fwrite(&BM[1], 1, 1, outputFile);
        int paddedRowSize = (int)(4 * ceil((float)width*bytesPerPixel/4.0f));
        int32 fileSize = paddedRowSize*height + HEADER_SIZE + INFO_HEADER_SIZE;
        fwrite(&fileSize, 4, 1, outputFile);
        int32 reserved = 0x0000;
        fwrite(&reserved, 4, 1, outputFile);
        int32 dataOffset = HEADER_SIZE+INFO_HEADER_SIZE;
        fwrite(&dataOffset, 4, 1, outputFile);
 
        //*******INFO*HEADER******//
        int32 infoHeaderSize = INFO_HEADER_SIZE;
        fwrite(&infoHeaderSize, 4, 1, outputFile);
        fwrite(&width, 4, 1, outputFile);
        fwrite(&height, 4, 1, outputFile);
        int16 planes = 1; //always 1
        fwrite(&planes, 2, 1, outputFile);
        int16 bitsPerPixel = bytesPerPixel * 8;
        fwrite(&bitsPerPixel, 2, 1, outputFile);
        //write compression
        int32 compression = NO_COMPRESION;
        fwrite(&compression, 4, 1, outputFile);
        //write image size (in bytes)
        int32 imageSize = width*height*bytesPerPixel;
        fwrite(&imageSize, 4, 1, outputFile);
        int32 resolutionX = 11811; //300 dpi
        int32 resolutionY = 11811; //300 dpi
        fwrite(&resolutionX, 4, 1, outputFile);
        fwrite(&resolutionY, 4, 1, outputFile);
        int32 colorsUsed = MAX_NUMBER_OF_COLORS;
        fwrite(&colorsUsed, 4, 1, outputFile);
        int32 importantColors = ALL_COLORS_REQUIRED;
        fwrite(&importantColors, 4, 1, outputFile);
        int i = 0;
        int unpaddedRowSize = width*bytesPerPixel;
        for ( i = 0; i < height; i++)
        {
                int pixelOffset = ((height - i) - 1)*unpaddedRowSize;
                fwrite(&pixels[pixelOffset], 1, paddedRowSize, outputFile);
        }
        fclose(outputFile);
}

int bmp_mirror(byte *ori,byte *tar, int32 xsize,int32 ysize,int32 pixel_perbyte){
    int avgX = (0+xsize)/2;
    for(int y=0;y!=ysize;++y){
        for(int x=0;x!=xsize;++x){
            int tarX = 2*avgX-(x+1);
            //R
            *(tar + pixel_perbyte*(y*xsize + tarX)+2) = *(ori + pixel_perbyte*(y*xsize + x)+2);
            //G
            *(tar + pixel_perbyte*(y*xsize + tarX)+1) = *(ori + pixel_perbyte*(y*xsize + x)+1);
            //B
            *(tar + pixel_perbyte*(y*xsize + tarX)+0) = *(ori + pixel_perbyte*(y*xsize + x)+0);
            
        }
    }
    return 0;
}
int bmp_padding(byte *ori, byte *tar, int32 xsize,int32 ysize,int32 pixel_perbyte,int K){

    for(int y=0;y!=ysize;++y){
        for(int x=0;x!=xsize;++x){
            //R
            *(tar + pixel_perbyte*((y+1)*(xsize+K-1) + (x+1))+2)= *(ori + pixel_perbyte*(y*xsize + x)+2);
            //G
            *(tar + pixel_perbyte*((y+1)*(xsize+K-1) + (x+1))+1)= *(ori + pixel_perbyte*(y*xsize + x)+1);
            //B
            *(tar + pixel_perbyte*((y+1)*(xsize+K-1) + (x+1))+0)= *(ori + pixel_perbyte*(y*xsize + x)+0);
            
            


        }
    }
    //填補第一行與最後一行
    for(int x=0;x<xsize+2;++x){
        *(tar + (pixel_perbyte*(x)+2)) = 0;
        *(tar + (pixel_perbyte*(x)+1)) = 255;
        *(tar + (pixel_perbyte*(x)+0)) = 0;
        *(tar + (pixel_perbyte*((ysize+1)*(xsize+K-1)+x)+2)) = 0;
        *(tar + (pixel_perbyte*((ysize+1)*(xsize+K-1)+x)+1)) = 255;
        *(tar + (pixel_perbyte*((ysize+1)*(xsize+K-1)+x)+0)) = 0;
    }
    //補第一列與最後一列
    for(int y=1;y<ysize+1;++y){

        *(tar + (pixel_perbyte*(y*(xsize+K-1))+2)) = 0;
        *(tar + (pixel_perbyte*(y*(xsize+K-1))+1)) = 255;
        *(tar + (pixel_perbyte*(y*(xsize+K-1))+0)) = 0;
        *(tar + (pixel_perbyte*(y*(xsize+K-1)+xsize+1)+2)) = 0;
        *(tar + (pixel_perbyte*(y*(xsize+K-1)+xsize+1)+1)) = 255;
        *(tar + (pixel_perbyte*(y*(xsize+K-1)+xsize+1)+0)) = 0;
    }


    return 0;
}

void print_array(int H,int W, float *result){
    for (int i = 0; i < H; i++) {
        printf("row[%d]:\t", i);
        for (int j = 0; j < W; j++) {
            printf("%f,\t",result[i*(W-2) + j] );
        }
        printf("\n");
    }
}
void print_char_array(int H,int W, unsigned char *result){
    for (int i = 0; i < H; i++) {
        printf("row[%d]:\t", i);
        for (int j = 0; j < W; j++) {
            printf("%d,\t",(uint8_t)result[i*(W-2) + j]);

        }
        printf("\n");
    }
}

int main(int argc, const char * argv[]) {
    byte *pixels;
    int32 width;
    int32 height;
    int32 bytesPerPixel;
    byte *target;
    
    ReadImage("/Users/ethan/Downloads/Xcode/read_image/read_image/dog.bmp", &pixels, &width, &height,&bytesPerPixel);
    WriteImage("/Users/ethan/Downloads/Xcode/Real_bmp_reader/Real_bmp_reader/original.bmp", pixels, width, height, bytesPerPixel);

    target = (byte*)malloc(bytesPerPixel*height*width);
    printf("pixelperbyte%d",bytesPerPixel);
    bmp_mirror(pixels, target, width, height,bytesPerPixel);
    WriteImage("/Users/ethan/Downloads/Xcode/Real_bmp_reader/Real_bmp_reader/mirror_dog.bmp", target, 240, 240, bytesPerPixel);

    byte *padded_image;
    int K=3; //K= filter size
    padded_image = (byte*)malloc(sizeof(byte)*(width+K-1)*(height+K-1)*bytesPerPixel);
    bmp_padding(pixels, padded_image, width, height, bytesPerPixel, K);
    WriteImage("/Users/ethan/Downloads/Xcode/Real_bmp_reader/Real_bmp_reader/padding_dog.bmp", padded_image, 242, 242, bytesPerPixel);

    
    

    return 0;
}
