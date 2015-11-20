#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*
  Taylor Okel
  Operating Systems 2015
  Adapted from Kevin Farley's solution with permission
*/

#define FNAME "interface"

int main(int argc, char **argv){
  int i, fd = 1, len;

  char ch, wr_buf[100], rd_buf[100];


  if(argc!=2 && argc!=3) {
    printf("Usage %s <read|write> [data]\n", argv[0]);
    exit(0);
  }

  fd = open("/dev/interface", O_RDWR); //Open file, allow read and write
  if(fd == -1){
    printf("Error opening file :(\nError: %d\n", errno);
    exit(-1);
  }

  if(argc == 2)
    len = 0;
  else if(argc == 3)
    len = strlen(argv[2]);

  //printf("len=%i\n", len);

  if(!strcmp("read", argv[1])){
    read(fd, rd_buf, sizeof(rd_buf));
    printf("Read device data: %s\n", rd_buf);
  }
  else if(!strcmp("write", argv[1])){
    strncpy(wr_buf, argv[2], len);
    write(fd, wr_buf, len);
    wr_buf[len] = '\0';
    //printf("len=%i\n", len);
    printf("Wrote to device: %s\n", wr_buf);
  }

  close(fd);

  return 0;
}
