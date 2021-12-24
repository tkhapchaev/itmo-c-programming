#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <stdlib.h>
#define TWO_BYTES unsigned short
#define FOUR_BYTES unsigned long

#pragma pack(push, 1)
typedef struct bitmap_header {
  TWO_BYTES Type;
  FOUR_BYTES Size;
  TWO_BYTES Reserved_1;
  TWO_BYTES Reserved_2;
  FOUR_BYTES OffsetBits;
}
BITMAP_FILE_HEADER;

#pragma pack(pop)
typedef struct bitmap_info {
  FOUR_BYTES Size;
  FOUR_BYTES Width;
  FOUR_BYTES Height;
  TWO_BYTES Planes;
  TWO_BYTES BitCount;
  FOUR_BYTES Compression;
  FOUR_BYTES SizeImage;
  FOUR_BYTES XPixelsPerMeter;
  FOUR_BYTES YPixelsPerMeter;
  FOUR_BYTES ColorUsed;
  FOUR_BYTES ColorImportant;
}
BITMAP_INFO_HEADER;

int count_neighbours(char ** data, int current_position_x, int current_position_y, int max_height, int max_width) {
  int neighbours = 0;
  int RI = current_position_x + 1;
  int RD = current_position_x - 1;
  int CI = current_position_y + 1;
  int CD = current_position_y - 1;
  if (RI == max_height)
    RI = 0;
  if (RD == -1)
    RD = max_height - 1;
  if (CI == max_width)
    CI = 0;
  if (CD == -1)
    CD = max_width - 1;
  neighbours += data[RD][CD] == 1;
  neighbours += data[RD][current_position_y] == 1;
  neighbours += data[RD][CI] == 1;
  neighbours += data[current_position_x][CD] == 1;
  neighbours += data[current_position_x][CI] == 1;
  neighbours += data[RI][CD] == 1;
  neighbours += data[RI][current_position_y] == 1;
  neighbours += data[RI][CI] == 1;
  return neighbours;
}

int main(int argc, char ** argv) {
  unsigned char out_filename[128];
  int next_dump = 0;
  char palette[8];
  int palette_size;
  BITMAP_FILE_HEADER bitmap_header;
  BITMAP_INFO_HEADER bitmap_info;
  char input[128] = {
    0
  };
  char output[128] = {
    0
  };
  int max_iterations = -1;
  int dump_frequency = 1;
  char ** data, ** previous, ** temporary;
  char * current_row;
  FILE * file_in;
  int cmd = 1;
  while (cmd < argc) {
    if (strcmp(argv[cmd], "--input") == 0) {
      cmd++;
      strcpy(input, argv[cmd]);
    }

    if (strcmp(argv[cmd], "--output") == 0) {
      cmd++;
      strcpy(output, argv[cmd]);
    }
    if (strcmp(argv[cmd], "--max_iter") == 0) {
      cmd++;
      sscanf(argv[cmd], "%d", & max_iterations);
    }

    if (strcmp(argv[cmd], "--dump_freq") == 0) {
      cmd++;
      sscanf(argv[cmd], "%d", & dump_frequency);
    }

    cmd++;
  }

  if (input[0] == 0 || output[0] == 0) return 0;
  _mkdir(output);
  file_in = fopen(input, "rb");
  if (file_in == NULL) return NULL;
  if (fread( & bitmap_header, 1, sizeof(bitmap_header), file_in) != sizeof(bitmap_header)) {
    fclose(file_in);
    return NULL;
  }

  if (bitmap_header.Type != 0x4D42) {
    printf("Error :: %s is not a .bmp file.\n", input);
    fclose(file_in);
    return NULL;
  }

  fread( & bitmap_info, 1, sizeof(BITMAP_INFO_HEADER), file_in) != sizeof(BITMAP_INFO_HEADER);
  palette_size = bitmap_header.OffsetBits - ftell(file_in);
  fread(palette, 1, palette_size, file_in);
  if (bitmap_info.Compression != 0) {
    printf("Error :: compression is not supported.\n");
    fclose(file_in);
    return NULL;
  }

  int current_string_length = (bitmap_header.Size - bitmap_header.OffsetBits) / bitmap_info.Height;
  if (bitmap_info.BitCount != 1) {
    printf("Error :: not a monochrome image.\n");
    fclose(file_in);
    return NULL;

  }

  current_row = malloc(current_string_length);
  data = malloc(bitmap_info.Height * sizeof(int * ));
  previous = malloc(bitmap_info.Height * sizeof(int * ));
  temporary = malloc(bitmap_info.Height * sizeof(int * ));
  for (int i = 0; i < bitmap_info.Height; i++) {
    data[i] = malloc(bitmap_info.Width);
    previous[i] = calloc(bitmap_info.Width, 1);
    temporary[i] = malloc(bitmap_info.Width);
    fseek(file_in, bitmap_header.OffsetBits + (bitmap_info.Height - i - 1) * current_string_length, SEEK_SET);
    fread(current_row, 1, current_string_length, file_in);
    for (int j = 0; j < bitmap_info.Width; j++) {
      data[i][j] = (current_row[j / 8] >> (7 - j % 8)) & 1;
    }
  }

  fclose(file_in);
  int current_iteration = 0;
  while (max_iterations == -1 || current_iteration < max_iterations) {
    unsigned int j;
    unsigned int k;
    unsigned int counter;
    for (j = 0; j < bitmap_info.Height && memcmp(data[j], previous[j], bitmap_info.Width) == 0; j++);
    if (j == bitmap_info.Height) {
      break;
    }

    for (j = 0, k = bitmap_info.Width; j < bitmap_info.Height && k == bitmap_info.Width; j++)
      for (k = 0; k < bitmap_info.Width && data[j][k] == ' '; k++);

    if (j == bitmap_info.Height) {
      break;
    }

    if (current_iteration == next_dump) {
      sprintf(out_filename, "%s\\%06d.bmp", output, current_iteration);
      file_in = fopen(out_filename, "wb");
      fwrite( & bitmap_header, 1, sizeof(bitmap_header), file_in);
      fwrite( & bitmap_info, 1, sizeof(bitmap_info), file_in);
      fwrite(palette, 1, palette_size, file_in);
      for (int i = 0; i < bitmap_info.Height; i++) {
        memset(current_row, 0, current_string_length);
        for (int j = 0; j < bitmap_info.Width; j++) {
          current_row[j / 8] |= data[bitmap_info.Height - 1 - i][j] << (7 - j % 8);
        }

        fwrite(current_row, 1, current_string_length, file_in);
      }

      fclose(file_in);
      next_dump += dump_frequency;
    }

    for (j = 0; j < bitmap_info.Height; j++) memcpy(previous[j], data[j], bitmap_info.Width);
    for (j = 0; j < bitmap_info.Height; j++) {
      for (k = 0; k < bitmap_info.Width; k++) {
        counter = count_neighbours(data, j, k, bitmap_info.Height, bitmap_info.Width);
        if (data[j][k] == 0) {
          if (counter == 3)
            temporary[j][k] = 1;
          else
            temporary[j][k] = 0;
        }

        if (data[j][k] == 1) {
          if (counter < 2 || counter > 3)
            temporary[j][k] = 0;
          else
            temporary[j][k] = 1;
        }
      }
    }

    for (j = 0; j < bitmap_info.Height; j++) memcpy(data[j], temporary[j], bitmap_info.Width);
    current_iteration++;
  }

  for (int i = 0; i < bitmap_info.Height; i++) {
    free(data[i]);
    free(previous[i]);
    free(temporary[i]);
  }

  free(data);
  free(previous);
  free(temporary);
  free(current_row);
  return 0;
}
