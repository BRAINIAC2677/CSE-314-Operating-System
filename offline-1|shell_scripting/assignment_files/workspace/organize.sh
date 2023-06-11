#!/bin/bash

submissions_folder_name=$1
targets_folder_name=$2
tests_folder_name=$3
answers_folder_name=$4
# what if arg number less than 4

get_file_name()
{
    dir=$1
    IFS="/"
    splitted_components=($dir)
    unset IFS
    echo ${splitted_components[-1]}
}

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
    file_name=$1
    IFS='.'
    splitted_components=($file_name)
    unset IFS 
    echo ${splitted_components[-1]}
}

# return value 0->not source code, 1->c file, 2->python file, 3->java file
get_extension_code()
{
    file_name="$1"
    file_extension=`get_extension "$file_name"`
    if [ "$file_extension" == "c" ]
    then 
        echo "1"
    elif [ "$file_extension" == "py" ]
    then 
        echo "2"
    elif [ "$file_extension" == "java" ]
    then 
        echo "3" 
    else 
        echo "0"
    fi 
}

get_source_code()
{
	if [ -d "$1" ]
	then
	
		for i in "$1"/*
		do
			get_source_code "$i"
		done
	
	elif [ -f "$1" ]
	then
        extension_code=`get_extension_code "$1"`
        if [ "$extension_code" != "0" ]
        then 
            echo "$1"
        fi 
	fi
}

generate_output_c()
{
    c_file="$1"
    test_dir="$2"
    gcc "$c_file" -o "main.out"
    counter=1
    for test in "$test_dir"/*
    do
        ./main.out < "$test" > "out${counter}.txt"
        counter=$( expr $counter + 1 )
    done
}

generate_output_py()
{
    py_file="$1"
    test_dir="$2"
    counter=1
    for test in "$test_dir"/*
    do
        python3 "$py_file" < "$test" > "out${counter}.txt"
        counter=$( expr $counter + 1 )
    done
}

generate_output_java()
{
    java_file="$1"
    test_dir="$2"
    javac "$java_file"
    counter=1
    for test in "$test_dir"/*
    do
        java Main < "$test" > "out${counter}.txt"
        counter=$( expr $counter + 1 )
    done
}

count_matched_unmatched()
{
    answer_dir=$1
    matched=0
    unmatched=0
    counter=1
    while [ true ]
    do  
        current_out_file="out${counter}.txt"
        current_answer_file="${answer_dir}/ans${counter}.txt"
        if [ ! -e "$current_out_file" ] || [ ! -e "$current_answer_file" ]
        then 
            break
        fi
        differences=`diff "$current_out_file" "$current_answer_file"`
        if [ "${#differences}" == "0" ]
        then 
            matched=$(( $matched + 1 ))
        else 
            unmatched=$(( $unmatched + 1 ))
        fi
        counter=$(( $counter + 1 ))
    done
    echo "$matched $unmatched"
}

# assumes currently in id folder
add_csv_entry()
{
    id=$1
    type=$2
    csv_file_path="$3"
    answer_dir="../../../${answers_folder_name}"
    count_arr=(`count_matched_unmatched "$answer_dir"`)
    matched=${count_arr[0]}
    unmatched=${count_arr[1]}
    echo "${id},${type},${matched},${unmatched}" >> "$csv_file_path" 
}

create_id_folder()
{
    id=$1
    source_file=$2
    cd "$targets_folder_name"
    extension_code=`get_extension_code "$source_file"`
    if [ "$extension_code" == "1" ]
    then 
        cd C 
        mkdir "$id"
        cd "$id"
        cp "../../../${submissions_folder_name}/$source_file" main.c 
        test_dir="../../../${tests_folder_name}"
        generate_output_c "main.c" "$test_dir"
        add_csv_entry "$id" "C" "../../../${targets_folder_name}/result.csv"
        cd ../..
    elif [ "$extension_code" == "2" ]
    then 
        cd Python 
        mkdir "$id"
        cd "$id"
        cp "../../../${submissions_folder_name}/$source_file" main.py 
        test_dir="../../../${tests_folder_name}"
        generate_output_py "main.py" "$test_dir"
        add_csv_entry "$id" "Python" "../../../${targets_folder_name}/result.csv"
       cd ../..
    elif [ "$extension_code" == "3" ]
    then 
        cd Java 
        mkdir "$id"
        cd "$id"
        cp "../../../${submissions_folder_name}/$source_file" Main.java 
        test_dir="../../../${tests_folder_name}"
        generate_output_java "Main.java" "$test_dir"
        add_csv_entry "$id" "Java" "../../../${targets_folder_name}/result.csv"
       cd ../..
    fi 
    cd ..
}

# create targets folder if doesn't exists
if ! [ -a "$targets_folder_name" ]
then 
    mkdir "$targets_folder_name"
    cd "$targets_folder_name"
    mkdir C Python Java
    touch result.csv
    echo "student_id,type,matched,not_matched" >> result.csv
    cd ..
fi

# task A
for file_name in "$submissions_folder_name"/*
do
    file_name=`get_file_name "$file_name"`
    echo $file_name
    id=`get_id "$file_name"`
    echo $id
    cd "$submissions_folder_name"
    mkdir "$id" 
    cp "$file_name" "$id"
    cd "$id" 
    unzip -qq "$file_name"
    cd ..
    source_file=`get_source_code "$id"`
    cd ..
    create_id_folder "$id" "$source_file"
    cd "$submissions_folder_name"
    rm -r "$id"
    cd ..
done

