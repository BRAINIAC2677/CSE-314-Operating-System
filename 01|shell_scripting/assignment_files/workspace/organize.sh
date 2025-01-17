#!/bin/bash

get_id()
{
    file_name=${1%????}
    IFS='_'
    splitted_components=($file_name)
    unset IFS 
    echo ${splitted_components[4]}
}

get_extension()
{
    filename=`basename "${1}"`
    echo ${filename##*.}
}

get_source_code_path()
{
	if [ -d "$1" ]; then
		for i in "$1"/*; do
			get_source_code_path "$i"
		done
	elif [ -f "$1" ]; then
        extension=`get_extension "$1"`
        if [ "$extension" == "c" ] || [ "$extension" == "py" ] || [ "$extension" == "java" ]; then 
            echo "$1"
        fi 
	fi
}

generate_and_match_output()
{
    id="$1"
    extension="$2"
    id_root_dir="${targets_folder_name}/${file_type[$extension]}/${id}"
    cd $id_root_dir
    ${compilation_commands[$extension]}
    counter=1
    matched=0
    for test in "${initial_folder}/${tests_folder_name}"/*; do
        current_out_file="out${counter}.txt"
        current_ans_file="${initial_folder}/${ans_folder_name}/ans${counter}.txt"
        ${run_commands[$extension]} < "$test" > "${current_out_file}"
        differences=`diff "${current_out_file}" "${current_ans_file}"`
        if [ "${#differences}" == "0" ]; then 
            matched=$(( $matched + 1 ))
        fi 
        counter=$( expr $counter + 1 )
    done
    cd "$initial_folder"
    echo "${id},${file_type[$extension]},${matched},$(( ${counter} - 1 - ${matched} ))" >> "${targets_folder_name}/result.csv" 
}

organize_file()
{
    file_path=$1
    file_name=`basename "$file_path"`
    id=`get_id "$file_name"`
    mkdir "$id" 
    cp "${submissions_folder_name}/${file_name}" "$id"
    unzip -qq "${id}/${file_name}" -d "${id}"
    source_code_path=`get_source_code_path "$id"`
    extension=`get_extension "$source_code_path"`
    if [  "$verbose" == "true" ]; then 
        echo "organizing files of $id"
    fi
    id_root_dir="${targets_folder_name}/${file_type[$extension]}/${id}"
    mkdir ${id_root_dir}
    cp "${source_code_path}" "${id_root_dir}/${desired_filename[$extension]}"
    rm -r "$id"
}

create_targets_folder()
{
    if [ -a "$targets_folder_name" ]; then 
        rm -r "$targets_folder_name"
    fi
    mkdir "$targets_folder_name"
    cd "$targets_folder_name"
    mkdir C Python Java
    if [ "$noexecute" == "false" ]; then
        touch result.csv
        echo "student_id,type,matched,not_matched" >> result.csv
    fi
    cd "$initial_folder"
}

main() 
{
    create_targets_folder
    for file_path in "$submissions_folder_name"/*; do
        organize_file "$file_path"
        if [ "$noexecute" == "false" ]; then
            if [ "$verbose" == "true" ]; then 
                echo "Executing files of $id"
            fi
            generate_and_match_output "$id" "$extension" 
        fi
    done
}

echo_man(){
    echo -e "\n
man page
---------\n
    NAME
        organize - organize, execute, generate output from test files and match output against answers for students submissions.\n
    SYNOPSIS
        ./ogranize arg1 arg2 arg3 arg4 [option]...\n
    DESCRIPTION
        organize, execute, generate output from test files and match output against answers for students submissions.\n
        arg1
            relative path of submissions folder\n
        arg2
            relative path of targets folder\n
        arg3
            relative path of tests folder\n
        arg4
            relative path of answers folder\n
        -v
            if provided, will print useful information while executing scripts.\n
        -noexecute
            if provided, will not execute the source code.\n
    AUTHOR
        Written by Asif Azad.
        "
}

if [ $# -lt 4 ]; then 
    echo -e "Missing arguments"
    echo_man
    exit 
fi

initial_folder=`pwd`
submissions_folder_name="$1"
targets_folder_name="$2"
tests_folder_name="$3"
ans_folder_name="$4"
verbose=false
noexecute=false
id=""
extension=""

declare -A compilation_commands=( [c]="gcc main.c -o main.out" [py]="" [java]="javac Main.java" )
declare -A run_commands=( [c]="./main.out" [py]="python3 main.py" [java]="java Main" )
declare -A file_type=( [c]="C" [py]="Python" [java]="Java" )
declare -A desired_filename=( [c]="main.c" [py]="main.py" [java]="Main.java" )

if [ ! -e "$submissions_folder_name" ]; then
  echo "Error, submission folder doesn't exist"
  exit
fi
if [ ! -e "$tests_folder_name" ]; then
  echo "Error, test folder doesn't exist"
  exit
fi
if [ ! -e "$ans_folder_name" ]; then
  echo "Error, answer folder doesn't exist"
  exit
fi

for arg in "$@"; do 
    if [ "$arg" == "-v" ]; then 
        verbose=true
    elif [ "$arg" == "-noexecute" ]; then
        noexecute=true
    elif [ "$arg" == "--help" ] || [ "$arg" == "-h" ]; then 
        echo_man 
    fi
done

main 