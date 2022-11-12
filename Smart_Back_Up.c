#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>

void file_copy(char* from_adress, char* to_adress) {
  struct stat *f_data = (struct stat *) malloc(sizeof(struct stat));
  struct stat *f_data__gz = (struct stat *) malloc(sizeof(struct stat));
  if (fopen(from_adress, "r") == NULL) {
    printf("Failed to open file");
    exit(EXIT_FAILURE);
  }
  char *gz_format = (char *) calloc(FILENAME_MAX, sizeof(char));
  strcpy(gz_format, to_adress);
  strcat(gz_format, ".gz");
  if (!fopen(gz_format, "r")) {//если копии еще не было
    pid_t p_id = fork();
    if (p_id == 0) {
      printf("Making a copy of %s\n", to_adress);
      execlp("cp", "cp", from_adress, to_adress, NULL);
    }
    waitpid(p_id, NULL, 0);
    p_id = fork();
    if (p_id == 0)
      execlp("gzip", "gzip", to_adress, NULL);
    exit(EXIT_SUCCESS);
  }
  stat(gz_format, f_data__gz);
  time_t gz_format_time = f_data__gz->st_ctime;
  stat(from_adress, f_data);
  time_t mod_time = f_data->st_mtime;//time of last modification
  free(f_data);                     //releasing f_data
  free(f_data__gz);
  if (gz_format_time < mod_time) {
    pid_t p_id = fork();
    if (p_id == 0) {
      printf("File rewriting %s\n", to_adress);
      execlp("cp", "cp", from_adress, to_adress, NULL);
    }
    waitpid(p_id, NULL, 0);
    p_id = fork();
    if (p_id == 0)
      execlp("rm", "rm", gz_format, NULL);
    waitpid(p_id, NULL, 0);
    p_id = fork();
    if (p_id == 0)
      execlp("gzip", "gzip", to_adress, NULL);
    waitpid(p_id, NULL, 0);
    exit(EXIT_SUCCESS);
  }
}

void check_and_copy(char* source_dir, char* destination_dir) {
  DIR* directory;
  struct dirent* file = NULL;
  char* from_adress = NULL;
  char* to_adress = NULL;
  if ((directory = opendir(source_dir)) == NULL) {
    printf("Исходной директории не существует\n");
    exit(EXIT_FAILURE);
  }
  while ((file = readdir(directory)) != NULL) {
    from_adress = (char *) calloc(FILENAME_MAX, sizeof(char));
    to_adress = (char *) calloc(FILENAME_MAX, sizeof(char));
    strcpy(from_adress, source_dir);
    strcat(from_adress, "/");
    strcat(from_adress, file->d_name);
    strcpy(to_adress, destination_dir);
    strcat(to_adress, "/");
    strcat(to_adress, file->d_name);
    struct stat* file_data = (struct stat*) malloc(sizeof(struct stat));
    stat(from_adress, file_data);
    if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0) {
      if (S_ISDIR(file_data->st_mode)) { //каталог ли
        if (opendir(to_adress) == NULL) {
          pid_t p_id = fork();
          if (p_id == 0) {
            execlp("mkdir", "mkdir", to_adress, (char *) NULL);
          }
          waitpid(p_id, NULL, 0);
        }
        check_and_copy(from_adress, to_adress);
      }
      if (S_ISREG(file_data->st_mode))//регулярный ли файл
        file_copy(from_adress, to_adress);
    }
  }
  closedir(directory);
}

int main (int argc, char* argv[]) {
  if (opendir(argv[1]) == NULL) {
    perror("Исходной директории не существует\n");
    exit(EXIT_FAILURE);
  }
  if (opendir(argv[2]) == NULL) {
    pid_t p_id = fork();
    if (p_id == 0) {
      execlp("mkdir", "mkdir", argv[2], (char *) NULL);
    }
    waitpid(p_id, NULL, 0);
  }
  check_and_copy(argv[1], argv[2]);
  return 0;
}