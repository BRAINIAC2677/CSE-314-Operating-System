#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include "simulator.h"

// inputs
int number_of_students;
int size_of_groups;
int printing_time;
int binding_time;
int entrybook_access_time;

// pre-defined variables
int number_of_printing_stations = 4;
int number_of_binding_stations = 2;
int number_of_staffs = 2;
double lambda_student = 4;
double lambda_staff = 9;

FILE *input_file;
FILE *output_file;

time_t start_time;

int number_of_groups;
int number_of_submissions;
int entrybook_reader_count;
int *student_states;

sem_t binding_station_semaphore;
sem_t *student_semaphores;
pthread_mutex_t student_state_mutex;
pthread_mutex_t entrybook_mutex;
pthread_mutex_t entrybook_reader_count_mutex;

Group *groups;
Staff *staffs;

int main()
{
    input_file = fopen("input.txt", "r");
    output_file = fopen("output.txt", "w");
    fscanf(input_file, "%d %d %d %d %d", &number_of_students, &size_of_groups, &printing_time, &binding_time, &entrybook_access_time);
    simulate();
    return 0;
}

void simulate()
{
    time(&start_time);
    mutex_init();
    group_init();
    staff_init();
    join_all_group_leader_threads();
    cancel_all_staff_threads();
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
    pthread_mutex_init(&entrybook_mutex, NULL);
    pthread_mutex_init(&entrybook_reader_count_mutex, NULL);
    sem_init(&binding_station_semaphore, 0, number_of_binding_stations);
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
            groups[i].students[j].print_arrival_time = get_poisson_randint(lambda_student);
            if (j == size_of_groups - 1)
            {
                pthread_create(&groups[i].students[j].thread, NULL, group_leader_startup, (void *)&groups[i].students[j]);
            }
            else
            {
                pthread_create(&groups[i].students[j].thread, NULL, group_member_startup, (void *)&groups[i].students[j]);
            }
        }
    }
}

void staff_init()
{
    staffs = (Staff *)malloc(sizeof(Staff) * number_of_staffs);
    for (int i = 0; i < number_of_staffs; i++)
    {
        staffs[i].id = i;
        staffs[i].reading_interval = get_poisson_randint(lambda_staff);
        pthread_create(&staffs[i].thread, NULL, staff_startup, (void *)&staffs[i]);
    }
}

void join_all_group_leader_threads()
{
    for (int i = 0; i < number_of_groups; i++)
    {
        pthread_join(groups[i].students[size_of_groups - 1].thread, NULL);
    }
}

void cancel_all_staff_threads()
{
    for (int i = 0; i < number_of_staffs; i++)
    {
        pthread_cancel(staffs[i].thread);
    }
}

void *group_member_startup(void *_arg)
{
    Student *student = (Student *)_arg;
    write(student);
    printing_phase(student);
    pthread_exit(0);
}

void *group_leader_startup(void *_arg)
{
    Student *student = (Student *)_arg;
    write(student);
    printing_phase(student);
    int group_id = get_group_id(student->id);
    binding_phase(&groups[group_id]);
    submission_phase(&groups[group_id]);
    pthread_exit(0);
}

void *staff_startup(void *_arg)
{
    Staff *staff = (Staff *)_arg;
    while (2677)
    {
        sleep(staff->reading_interval);
        pthread_mutex_lock(&entrybook_reader_count_mutex);
        entrybook_reader_count++;
        if (entrybook_reader_count == 1)
        {
            pthread_mutex_lock(&entrybook_mutex);
        }
        pthread_mutex_unlock(&entrybook_reader_count_mutex);
        read_entrybook(staff->id);
        pthread_mutex_lock(&entrybook_reader_count_mutex);
        entrybook_reader_count--;
        if (entrybook_reader_count == 0)
        {
            pthread_mutex_unlock(&entrybook_mutex);
        }
        pthread_mutex_unlock(&entrybook_reader_count_mutex);
    }
}

void write(Student *_student)
{
    sleep(_student->print_arrival_time);
}

void printing_phase(Student *_student)
{
    int printing_station_id = _student->id % number_of_printing_stations;
    fprintf(output_file, "Student %d has arrived at the print station %d at time %d.\n", _student->id + 1, printing_station_id + 1, get_current_time());
    enter_printing_station(_student->id);
    fprintf(output_file, "Student %d has started printing at the printing station %d at time %d.\n", _student->id + 1, printing_station_id + 1, get_current_time());
    print();
    fprintf(output_file, "Student %d has finished printing at printing station %d at time %d.\n", _student->id + 1, printing_station_id + 1, get_current_time());
    exit_printing_station(_student->id);
}

void binding_phase(Group *_group)
{
    for (int i = 0; i < size_of_groups - 1; i++)
    {
        pthread_join(_group->students[i].thread, NULL);
    }
    fprintf(output_file, "Group %d has arrived at the binding station at time %d.\n", get_group_id(_group->students[0].id) + 1, get_current_time());
    sem_wait(&binding_station_semaphore);
    fprintf(output_file, "Group %d has started binding at time %d.\n", get_group_id(_group->students[0].id) + 1, get_current_time());
    bind();
    fprintf(output_file, "Group %d has finished binding at time %d.\n", get_group_id(_group->students[0].id) + 1, get_current_time());
    sem_post(&binding_station_semaphore);
}

void submission_phase(Group *_group)
{
    int group_id = get_group_id(_group->students[0].id);
    fprintf(output_file, "Group %d has arrived at the library at time %d.\n", group_id + 1, get_current_time());
    pthread_mutex_lock(&entrybook_mutex);
    fprintf(output_file, "Group %d has started submission at time %d.\n", group_id + 1, get_current_time());
    submit();
    fprintf(output_file, "Group %d has finished submission at time %d.\n", group_id + 1, get_current_time());
    pthread_mutex_unlock(&entrybook_mutex);
}

void read_entrybook(int _staff_id)
{
    fprintf(output_file, "Staff %d has started reading entry book at time %d.\n", _staff_id + 1, get_current_time());
    sleep(entrybook_access_time);
    fprintf(output_file, "Staff %d has finished reading the entry book at time %d. No. of submission = %d\n", _staff_id + 1, get_current_time(), number_of_submissions);
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
    for (int i = _student_id % number_of_printing_stations; i < number_of_students; i += number_of_printing_stations)
    {
        if (in_same_group(_student_id, i))
        {
            test(i);
        }
    }
    for (int i = _student_id % number_of_printing_stations; i < number_of_students; i += number_of_printing_stations)
    {
        if (!in_same_group(_student_id, i))
        {
            test(i);
        }
    }
    pthread_mutex_unlock(&student_state_mutex);
}

void bind()
{
    sleep(binding_time);
}

void submit()
{
    sleep(entrybook_access_time);
    number_of_submissions++;
}

void test(int _student_id)
{
    int anyone_printing = 0;
    for (int i = _student_id % number_of_printing_stations; i < number_of_students; i += number_of_printing_stations)
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

int get_current_time()
{
    time_t current_time;
    time(&current_time);
    int elapsed_time = difftime(current_time, start_time);
    return elapsed_time;
}

int get_group_id(int _student_id)
{
    return _student_id / size_of_groups;
}

int get_poisson_randint(double _lambda)
{
    double limit = exp(-_lambda);
    int k = 0;
    double p = 1;
    for (; p > limit; k++)
    {
        p *= (double)rand() / (double)RAND_MAX;
    }
    return k - 1;
}