#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define WIDTH 80
#define HEIGHT 40
#define BUF_SIZE ((WIDTH + 1) * HEIGHT + 1)

#define BITMAP_SIGNATURE 0x4d42

#pragma pack(push, 1)

typedef struct _BITMAP_FILEHEADER {
  uint16_t Signature;
  uint32_t Size;
  uint32_t Reserved;
  uint32_t BitsOffset;
} BITMAP_FILEHEADER;

#define BITMAP_FILEHEADER_SIZE 14

typedef struct _BITMAP_HEADER {
  uint32_t HeaderSize;
  int32_t Width;
  int32_t Height;
  uint16_t Planes;
  uint16_t BitCount;
  uint32_t Compression;
  uint32_t SizeImage;
  int32_t PelsPerMeterX;
  int32_t PelsPerMeterY;
  uint32_t ClrUsed;
  uint32_t ClrImportant;
} BITMAP_HEADER;

#pragma pack(pop)

double luminance(int r, int g, int b) {
  return (double)(0.2126 * r + 0.7152 * g + 0.0722 * b);
}

char ascii(int r, int g, int b) {
  double max, min, cur;

  max = luminance(255, 255, 255);
  min = luminance(0, 0, 0);
  cur = luminance(r, g, b);

  if (cur <= 0.2 * (max - min) - min) {
    return 'M';
  }
  return ' ';
}

int main(int argc, char **argv) {
  BITMAP_FILEHEADER fileheader;
  BITMAP_HEADER header;
  int fd;
  ssize_t nRead;
  off_t off;
  int linewidth;
  int interwidth, interheight;
  unsigned char *line;
  unsigned char *curr;
  unsigned char r, g, b;
  char buf[BUF_SIZE];
  char *bufP = buf;

  fd = open(argv[1], O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  nRead = read(fd, &fileheader, BITMAP_FILEHEADER_SIZE);
  if (nRead == -1) {
    perror("read fileheader");
    exit(EXIT_FAILURE);
  }

  if (fileheader.Signature != BITMAP_SIGNATURE) {
    write(STDERR_FILENO, "signature wrong\n", 16);
    exit(EXIT_FAILURE);
  }

  nRead = read(fd, &header, sizeof(BITMAP_HEADER));
  if (nRead == -1) {
    perror("read header");
    exit(EXIT_FAILURE);
  }

  if (header.HeaderSize != 40) {
    write(STDERR_FILENO, "warn: not BITMAPINFOHEADER\n", 27);
  }
  if (header.Compression != 0) {
    write(STDERR_FILENO, "warn: Compression not 0\n", 24);
    exit(EXIT_FAILURE);
  }
  if (header.ClrUsed != 0) {
    write(STDERR_FILENO, "warn: ClrUsed not 0\n", 20);
    exit(EXIT_FAILURE);
  }

  linewidth = ((header.Width * header.BitCount / 8) + 3) & ~3;
  line = malloc(linewidth);
  if (line == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  interwidth = header.Width / WIDTH;
  interheight = header.Height / HEIGHT;

  for (int i = 0; i < header.Height; i++) {
    off = lseek(fd, fileheader.BitsOffset + (header.Height - i - 1) * linewidth,
                SEEK_SET);
    if (off == -1) {
      perror("lseek: BitsOffset");
      exit(EXIT_FAILURE);
    }

    nRead = read(fd, line, linewidth);
    if (nRead == -1) {
      perror("read line");
      exit(EXIT_FAILURE);
    }

    curr = line;
    for (int j = 0; j < header.Width; j++) {
      if (header.BitCount == 24) {
        b = *curr++;
        g = *curr++;
        r = *curr++;

        if (j % interwidth == 0 && i % interheight == 0) {
          snprintf(bufP++, BUF_SIZE, "%c", ascii(r, g, b));
        }
      }
    }
    if (i % interheight == 0) {
      snprintf(bufP++, BUF_SIZE, "\n");
    }
  }
  buf[BUF_SIZE - 1] = '\0';

  printf(buf);
  free(line);
}
