#!/bin/bash

mode="$1"

source_file="/home/opam/demo/problems/cs1_org.c"			#Source file path - absolute file path.
entry_func="p"								#Entry point of the main function to add assertions to.
slice_var="output"							#Variable with respect to which slicing is to be done.
safety_cond_file=""							#Absolute path to the .txt file condition the safety condition
#instrument_seu_path="/home/opam/demo/instrument_seu"			#Path to the instrumentation executable.

if [ "$mode" != "testing" ]; then
	echo "Enter the absolute file path to source file"
	read source_file

	echo "Enter the entry function in consideration"
	read entry_func

	echo "Enter the variable w.r.t slice the entry function"
	read slice_var

	echo "Enter the absolute file path to the txt file containing the safety condition"
	read safety_cond_file

	echo "Type the absolute path to the 'instrument_seu' executable"
	read instrument_seu_path
fi

filepath="${source_file%.c}"
filename=$(basename "$filepath")
output_path=$(pwd)
output_path="${output_path}/output_${filename}/"
mkdir -p "$output_path"
sliced_file="${output_path}${filename}_frama_sliced.c"					#Filename for output of Frama static analysis.
sliced_instrumented_file="${output_path}${filename}_instru.c"				#Filename for instrumented code.
sliced_instrumented_cleaned_file="${output_path}${filename}_instru_cleaned.c"		#Filename for instrumented and cleaned code.
final_output_file="${output_path}${filename}_cbmc_ready.c"				#Filename for the final file to be used by cbmc.

#Switching to the frama-switch, performing static analysis using frama.
echo "[+] Switching to Frama-C OPAM Switch..."
eval $(opam env --switch=frama-switch --set-switch)
frama-c -load-module slicing "${source_file}" -main "${entry_func}" -slice-value "${slice_var}" -slicing-level 3 -then-on 'Slicing export' -print -ocode "${sliced_file}" > /dev/null 2>&1
echo "Static slicing performed, file stored at ${sliced_file}"


#Switching to the cil-switch, performing instrumentation, cleaning.
echo "[+] Switching to CIL OPAM Switch..."
eval $(opam env --switch=cil-switch --set-switch)
	#Need to automate the process of detecting all the variables with respect to which the crv detection is to be done. USING USER INPUT FOR NOW.
echo "What variable to check instrumentation of"
variable="x"
if [ "$mode" != "testing" ]; then
	read variable
fi

#Creating the instrumentation executable.
ocamlfind ocamlopt -package cil -linkpkg -o instrument_seu /home/opam/demo/instrument_seu.ml > /dev/null 2>&1

./instrument_seu "${sliced_file}" "${sliced_instrumented_file}" "${variable}"
echo "Finished instrumenting the sliced code"

gcc -E -P "${sliced_instrumented_file}" -o  "${sliced_instrumented_cleaned_file}" > /dev/null 2>&1

#Appending '_sliced' to the function name in the cleaned file.
sed -i -E 's/^([A-Za-z]+[[:space:]]+)([A-Za-z0-9_]+)\(/\1\2_prime(/' "${sliced_instrumented_cleaned_file}" > /dev/null 2>&1
echo "Finished instrumentation and cleaning. File available at ${sliced_instrumented_cleaned_file}"


#Creating the final cbmc ready file.
cp "${source_file}" "${final_output_file}"
echo -e "\n\n// ----- Renamed Instrumented Function -----\n" >> "${final_output_file}"
sed -i 's/\r$//' "${final_output_file}"
cat "${sliced_instrumented_cleaned_file}" >> "${final_output_file}"

echo "[+] CBMC ready file created: ${final_output_file}"
