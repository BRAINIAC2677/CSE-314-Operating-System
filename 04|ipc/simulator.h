#include <pthread.h>

#define WRITING 0
#define WAITING 1
#define PRINTING 2

typedef struct
{
    int id;
    int print_arrival_time;
    pthread_t thread;
} Student;

typedef struct
{
    Student *students;
} Group;

typedef struct
{
    int id;
    int reading_interval;
    pthread_t thread;
} Staff;

// forward declarations
void simulate();
void mutex_init();
void group_init();
void staff_init();
void join_all_group_leader_threads();
void cancel_all_staff_threads();
void *group_member_startup(void *_arg);
void *group_leader_startup(void *_arg);
void *staff_startup(void *_arg);
void write(Student *_student);
void printing_phase(Student *_student);
void binding_phase(Group *_group);
void submission_phase(Group *_group);
void read_entrybook(int _staff_id);
void enter_printing_station(int _student_id);
void print();
void exit_printing_station(int _student_id);
void bind();
void submit();
void test(int _student_id);
int in_same_group(int _student_id_1, int _student_id_2);
int get_current_time();
int get_group_id(int _student_id);
int get_poisson_randint(double _lambda);
