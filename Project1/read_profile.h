#ifndef READ_PROFILE
#define READ_PROFILE

#define PROFILE ".myprofile"
#define HOME_ENV_NAME "HOME"
#define PATH_ENV_NAME "PATH"

void change_home_directory();

int start_with(const char* line, char *name, int *position);

void path_value(char *str);

void home_value(char *str);

int fill_variables(char* line);

int read_file(char *filename);

int read_profile();


#endif
