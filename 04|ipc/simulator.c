#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

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

time_t start_time;

FILE *input_file;
FILE *output_file;

int number_of_students;
int number_of_groups;
int size_of_groups;
int printing_time;
int binding_time;
int entrybook_access_time;

int number_of_printers = 1;
int *student_states;
pthread_mutex_t student_state_mutex;
sem_t *student_semaphores;
Group *groups;

void *group_member_startup(void *_arg);
void write(Student *_student);
void printing_phase(Student *_student);
int get_current_time();
void enter_printing_station(int _student_id);
void print();
void exit_printing_station(int _student_id);
void test(int _student_id);
int in_same_group(int _student_id_1, int _student_id_2);
void simulation_init();
void mutex_init();
void group_init();
int get_randint();
void join_all_threads();

void *group_member_startup(void *_arg)
{
    Student *student = (Student *)_arg;
    write(student);
    printing_phase(student);
    pthread_exit(0);
}

void write(Student *_student)
{
    sleep(_student->print_arrival_time);
}

void printing_phase(Student *_student)
{
    fprintf(output_file, "Student %d has arrived at the print station at time %d.\n", _student->id + 1, get_current_time());
    // printf("Student %d has arrived at the print station at time %d.\n", _student->id, get_current_time());
    enter_printing_station(_student->id);
    fprintf(output_file, "Student %d has started printing at time %d.\n", _student->id + 1, get_current_time());
    // printf("Student %d has started printing at time %d.\n", _student->id, get_current_time());
    print();
    fprintf(output_file, "Student %d has finished printing at time %d.\n", _student->id + 1, get_current_time());
    // printf("Student %d has finished printing at time %d.\n", _student->id, get_current_time());
    exit_printing_station(_student->id);
}

int get_current_time()
{
    time_t current_time;
    time(&current_time);
    int elapsed_time = difftime(current_time, start_time);
    return elapsed_time;
}

void enter_printing_station(int _student_id)
{
    pthread_mutex_lock(&student_state_mutex);
    student_states[_student_id] = WAITING;
    test(_student_id);
    pthread_mutex_unlock(&student_state_mutex);
    sem_wait(&student_semaphores[_student_id]);
}

void print()
{
    sleep(printing_time);
}

void exit_printing_station(int _student_id)
{
    pthread_mutex_lock(&student_state_mutex);
    student_states[_student_id] = WRITING;
    for (int i = _student_id % number_of_printers; i < number_of_students; i += number_of_printers)
    {
        if (in_same_group(_student_id, i))
        {
            test(i);
        }
    }
    for (int i = _student_id % number_of_printers; i < number_of_students; i += number_of_printers)
    {
        if (!in_same_group(_student_id, i))
        {
            test(i);
        }
    }
    pthread_mutex_unlock(&student_state_mutex);
}

void test(int _student_id)
{
    int anyone_printing = 0;

    for (int i = _student_id % number_of_printers; i < number_of_students; i += number_of_printers)
    {
        if (student_states[i] == PRINTING)
        {
            anyone_printing = 1;
            break;
        }
    }

    if (student_states[_student_id] == WAITING && !anyone_printing)
    {
        student_states[_student_id] = PRINTING;
        sem_post(&student_semaphores[_student_id]);
    }
}

int in_same_group(int _student_id_1, int _student_id_2)
{
    return _student_id_1 / size_of_groups == _student_id_2 / size_of_groups;
}

void simulation_init()
{
    mutex_init();
    group_init();
}

void mutex_init()
{
    student_states = (int *)malloc(sizeof(int) * number_of_students);
    student_semaphores = (sem_t *)malloc(sizeof(sem_t) * number_of_students);
    for (int i = 0; i < number_of_students; i++)
    {
        student_states[i] = WRITING;
        sem_init(&student_semaphores[i], 0, 0);
    }
    pthread_mutex_init(&student_state_mutex, NULL);
}

void group_init()
{
    number_of_groups = number_of_students / size_of_groups;
    groups = (Group *)malloc(sizeof(Group) * (number_of_groups));
    for (int i = 0; i < number_of_groups; i++)
    {
        groups[i].students = (Student *)malloc(sizeof(Student) * size_of_groups);
        for (int j = 0; j < size_of_groups; j++)
        {
            groups[i].students[j].id = i * size_of_groups + j;
            groups[i].students[j].print_arrival_time = get_randint();
            pthread_create(&groups[i].students[j].thread, NULL, group_member_startup, (void *)&groups[i].students[j]);
        }
    }
}

int get_randint()
{
    int min = 1;
    int max = 20;
    return (rand() % (max - min + 1)) + min;
}

void join_all_threads()
{
    for (int i = 0; i < number_of_groups; i++)
    {
        for (int j = 0; j < size_of_groups; j++)
        {
            pthread_join(groups[i].students[j].thread, NULL);
        }
    }
}

int main()
{
    time(&start_time);

    input_file = fopen("input.txt", "r");
    output_file = fopen("output.txt", "w");

    fscanf(input_file, "%d %d %d %d %d", &number_of_students, &size_of_groups, &printing_time, &binding_time, &entrybook_access_time);

    simulation_init();
    join_all_threads();

    return 0;
}