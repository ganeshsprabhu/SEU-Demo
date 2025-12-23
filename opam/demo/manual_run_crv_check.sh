#!/bin/bash

mode="$1"

source_file="/home/opam/demo/problems/cs1_org.c"			#Source file path - absolute file path.
entry_func="p"								#Entry point of the main function to add assertions to.
slice_var="output"							#Variable with respect to which slicing is to be done.
line_num=34								#The line at which entry func is called. Safety condition is inserted right below this line.
safety_cond_file=""							#Absolute path to the .txt file condition the safety condition

#Output files get created in following folders:
# /home/opam/demo/problems/cs1_org			=>is the main output dir, contains sliced file, special file with mappings (var_name, identity_num).
# /home/opem/demo/problems/cs1_org/<var_num>/		=>will contain the instrumented, cleaned, cbmc ready files specific to the variable under inspection.


################# STEP 1: Getting the Path to Source File ####################
if [ "$mode" != "testing" ]; then
	echo "Enter the absolute file path to source file"
	read source_file
fi
output_dir="${source_file%.c}"
mkdir -p "$output_dir"
echo "Successfully created output directory: ${output_dir}"

################# STEP 2: Creating the Sliced File ###########################
if [ "$mode" != "testing" ]; then
	echo "Enter the entry function in consideration"
	read entry_func

	echo "Enter the variable w.r.t slice the entry function"
	read slice_var
fi
filename=$(basename "$output_dir")
sliced_file="${output_dir}/${filename}_frama_sliced.c"
echo "[+] Switching to Frama-C OPAM Switch..."
eval $(opam env --switch=frama-switch --set-switch)
frama-c -load-module slicing "${source_file}" -main "${entry_func}" -slice-value "${slice_var}" -slicing-level 3 -then-on 'Slicing export' -print -ocode "${sliced_file}" > /dev/null 2>&1
echo "Static slicing performed, file stored at ${sliced_file}"


################# STEP 3: Changing to CIL Switch #############################
echo "[+] Switching to CIL OPAM Switch..."
eval $(opam env --switch=cil-switch --set-switch)
echo "Changed to CIL OPAM Switch"



################# STEP 4: Listing unique variables in entry function, writing mappings to unique numbers ######################
tmp_uniq_vars_txt="${output_dir}/tmp_uniq_vars.txt"
final_uniq_vars_txt="${output_dir}/uniq_vars.txt"

ocamlfind ocamlopt -package cil -linkpkg -o list_vars /home/opam/demo/list_vars.ml > /dev/null 2>&1
./list_vars "${sliced_file}" "${entry_func}" "${tmp_uniq_vars_txt}"
echo "Temporary txt file with unique variable names written to: ${tmp_uniq_vars_txt}"

#writing mappings to the final txt file.
mapfile -t vars < "$tmp_uniq_vars_txt"
: > "$final_uniq_vars_txt"
for i in "${!vars[@]}"; do
    printf "vars[%d] = %s\n" "$i" "${vars[$i]}" >> "$final_uniq_vars_txt"
done

rm -f "$tmp_uniq_vars_txt"
echo "Final indexed variables written to: ${final_uniq_vars_txt}"



################# STEP 5: Creating the Instrumentation Executable, instrumented code, cbmc ready code ######################
ocamlfind ocamlopt -package cil -linkpkg -o instrument_seu /home/opam/demo/instrument_seu.ml > /dev/null 2>&1
#iterating on all the folders(the numbers) and then creating the instrumented file within that folder.
for i in "${!vars[@]}";do
	folder_path="${output_dir}/${i}"
	mkdir -p "${folder_path}"
	echo "folder created: ${folder_path}"

	instru="${folder_path}/${filename}_instru.c"				#Filename for instrumented code.
	instru_clean="${folder_path}/${filename}_instru_cleaned.c"		#Filename for instrumented and cleaned code.
	final_output="${folder_path}/${filename}_cbmc_start.c"			#Filename for the final file to be used by cbmc.

	./instrument_seu "${sliced_file}" "${instru}" "${vars[$i]}"
	echo "Finished instrumentation, sliced_file used: ${sliced_file}, created: ${instru}, variable instrumented: ${vars[$i]}"

	gcc -E -P "${instru}" -o  "${instru_clean}" > /dev/null 2>&1

	#Appending '_sliced' to the function name in the cleaned file.
	sed -i -E 's/^([A-Za-z]+[[:space:]]+)([A-Za-z0-9_]+)\(/\1\2_prime(/' "${instru_clean}" > /dev/null 2>&1
	echo "Finished instrumentation and cleaning. File available at ${instru_clean}"


	#Creating the final cbmc ready file.
	cp "${source_file}" "${final_output}"
	awk '
	  /^#include/ { last_include = NR }
	  { lines[NR] = $0 }
	  END {
	    for (i = 1; i <= NR; i++) {
	      print lines[i]
	      if (i == last_include) {
		print "#include \"simulate_seu.h\""
		print "#include \"queue.h\""
	      }
	    }
	  }
	' "${final_output}" > "${final_output}.tmp" && mv "${final_output}.tmp" "${final_output}"

	awk '
	  BEGIN {
	    in_main = 0
	    inserted = 0
	  }

	  # Detect main function signature
	  /^[[:space:]]*(int|void)[[:space:]]+main[[:space:]]*\(/ {
	    in_main = 1
	  }

	  {
	    print

	    # Insert immediately after the opening brace of main
	    if (in_main && !inserted && /\{/) {
	      print "    Queue q1;"
	      print "    initQueue(&q1);"
	      print "    Queue q2;"
	      print "    initQueue(&q2);"
	      print ""
	      inserted = 1
	      in_main = 0
	    }
	  }
	' "${final_output}" > "${final_output}.tmp" && mv "${final_output}.tmp" "${final_output}"

	echo -e "\n\n// ----- Renamed Instrumented Function -----\n" >> "${final_output}"
	sed -i 's/\r$//' "${final_output}"
	cat "${instru_clean}" >> "${final_output}"

	echo "[+] CBMC ready file created: ${final_output}"
done


################# STEP 7: Adding the Safety Condition Relevant file into CBMC Ready File ###################################
if [ "$mode" != "testing" ]; then
	echo "Enter the line number in file: ${source_file} at which the entry function is being called"
	read line_num 
	line_num=line_num+2		#Add '2' to account for insertion of the 2 include .h lines.

	echo "Enter the path to the safety condition.txt file"
	read safety_cond_file
fi
for i in "${!vars[@]}";do
	folder_path="${output_dir}/${i}"
	final_cbmc_ready="${folder_path}/${filename}_cbmc_ready.c"
done
