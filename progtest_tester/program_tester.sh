#!/bin/bash

#LOCAL VARIABLES
YE='\033[1;93m'
CY='\033[1;36m'
RED='\033[1;31m'
GR='\033[1;32m'
NC='\033[0m'
PUR='\033[1;35m'
EXIST=0;
NUM_OF_FILES=0;
c=0;
SCORE=0;
LINE=0;
h=-1;
SUPPRESS_SANITIZER=0;

rm -f diff_outputs.txt;
echo;

path_set () {
	if  [ -f .program-tester.conf ];then
		# remove from config file last PATH_TO_PROGTEST variable
		if ! [ $(grep .program-tester.conf -c -e "PATH_TO_PROGTEST") == 0 ] ; then
			LINE="$(grep -no -m 1 -F .program-tester.conf -e "PATH_TO_PROGTEST" | grep -Eo '[0-9]{1,}')";
			echo -e "\n (${RED}Warning!${NC})\n${CY} Old path detected, remove old path?${NC}";printf "(y/n):";
			read CHOICE;
			if [ "$CHOICE" == 'y' ]; then
				sed -i "${LINE}d" .program-tester.conf; 
				echo -e "\n  ${YE}Config CLEANED!${NC}";echo;
			
			else
				echo -e "\n${YE}   Please remove ${CY}PATH_TO_PROGTEST${YE} from ${NC}.program-tester.conf${YE} file.${NC}\n";
				sleep 0.5;
				exit;
			fi
		fi

	fi

	echo -e "${CY}(${NC}Config option${CY})\n${YE}Please enter path to Progtest program source code ${NC}(${CY}.c file${NC})${YE}:${NC} ";
	read PATH_TO_PROGTEST;
	if [ -z "$PATH_TO_PROGTEST" ];then 
		exit;
	fi
	FILE_NAME=$(basename $PATH_TO_PROGTEST);
	
	PATH_TO_PROGTEST=$(eval printf "$PATH_TO_PROGTEST");
       if [ -f ${PATH_TO_PROGTEST} ]; then	       
	       	if ! [[ ${FILE_NAME} == *.c ]]; then
			echo -e "\n(${RED}Not .c file!${NC})\n";
			exit;
		fi
	       	printf "PATH_TO_PROGTEST=${PATH_TO_PROGTEST}\n" >> .program-tester.conf;
		export PATH_TO_PROGTEST="${PATH_TO_PROGTEST}";
		echo -e "${CY}\n  PATH SET!\n${NC}";
		sleep 0.3;
       fi
       if [ ! -f ${PATH_TO_PROGTEST} ] || [ -z ${PATH_TO_PROGTEST} ]; then
	       echo -e "(${RED}Incorrect path!${NC})"; sleep 0.2;
		exit;
       fi
}

#If confing file not exist create
if [ ! -f .program-tester.conf ]; then
	touch .program-tester.conf
	path_set;
	echo "SUPPRESS_SANITIZER=0" >> .program-tester.conf
fi

#Load stored config
source ./.program-tester.conf;

#If in config is no var PATH_TO_PROGTEST
if [ $(grep .program-tester.conf -c -e "PATH_TO_PROGTEST") == 0 ]; then
	path_set;
fi

#Add DEBUG option to config
if [ $(grep -cE "^DEBUG=" .program-tester.conf) == 0 ]; then
	echo "DEBUG=0" >> .program-tester.conf
	DEBUG=0
fi

#Add DEBUG option to config
if [ $(grep -cE "^DISABLE_VALGRIND=" .program-tester.conf) == 0 ]; then
	echo "DISABLE_VALGRIND=0" >> .program-tester.conf
	DISABLE_VALGRIND=0
fi

#If in config not found SUPPRESS_SANITIZER set opt
if [ $(grep -cE "^SUPPRESS_SANITIZER=" .program-tester.conf) == 0 ]; then
	echo -e "${CY}(${NC}Config option${CY})\n${YE}Do you want to use fsaniter?${NC}"
	printf "(y/n):"
	read CHOICE
        if [ "$CHOICE" != 'y' ];then
                CHOICE=1
        fi
        if [ "$CHOICE" == 'y' ];then
                CHOICE=0
        fi
	SUPPRESS_SANITIZER=$CHOICE
	echo "SUPPRESS_SANITIZER=$CHOICE" >> .program-tester.conf
	CONF_CHANGE=1
	echo
fi

#Set report config option
if [ $(grep -cE "^EXPANDED_REPORT=" .program-tester.conf) == 0 ]; then
	echo -e "${CY}(${NC}Config option${CY})\n${YE}Do you want to see full program report?${NC}"
	printf "(y/n):"
	read CHOICE
	if [ "$CHOICE" != 'y' ];then
		CHOICE=0
	fi
	if [ "$CHOICE" == 'y' ];then
		CHOICE=1
	fi
	EXPANDED_REPORT=$CHOICE
	echo "EXPANDED_REPORT=$CHOICE" >> .program-tester.conf
	echo;
	CONF_CHANGE=1
fi

#Set order dependent testing
if [ $(grep -cE "^STRICT_ORDER=" .program-tester.conf) == 0 ]; then
	echo -e "${CY}(${NC}Config option${CY})\n${YE}Do order of output matter?${NC}"
	printf "(y/n):"
	read CHOICE
	if [ "$CHOICE" != 'n' ];then
		CHOICE=1
	fi
	if [ "$CHOICE" == 'n' ];then
		CHOICE=0
	fi
	STRICT_ORDER=$CHOICE
	echo "STRICT_ORDER=$CHOICE" >> .program-tester.conf
	echo;
	CONF_CHANGE=1
fi

#Optimized report config setting
if [ $(grep -cE "^OPTIMIZED_REPORT=" .program-tester.conf) == 0 ]; then
        echo -e "${CY}(${NC}Config option${CY})\n${YE}Do you want to see program report with optimization flag?\n${CY}info:${NC} Compiler may \"merge\" some functions and they may not be visible after opt...${NC}"
        printf "(y/n):"
        read CHOICE
        if [ "$CHOICE" != 'y' ];then
                CHOICE=0
        fi
        if [ "$CHOICE" == 'y' ];then
                CHOICE=1
        fi
        OPTIMIZED_REPORT=$CHOICE
        echo "OPTIMIZED_REPORT=$CHOICE" >> .program-tester.conf
        echo;
	CONF_CHANGE=1
fi

if [ "$CONF_CHANGE" == '1' ];then
	echo -e "\n${CY}  You can change config settings later in${NC} .program-tester.conf ${CY}file.\n"
	sleep 3
fi


# SET PATH TO PROGTEST IF NOT EXIST
if [ ! -f ${PATH_TO_PROGTEST} ] || [ -z ${PATH_TO_PROGTEST} ] ; then
	echo -e "\n (${RED}File not found${NC})\n > Please make sure typed PATH is correct\n"
		path_set;
	fi

# Menu
echo -e "${YE}      Auto test of progtest program for reference input\n${NC}";
echo -e "\n Testing on: ${CY}${PATH_TO_PROGTEST}${NC}\n"


# Ccount input files
NUM_OF_FILES="$(echo *_in.txt | wc -w)"
if [ "$(echo *_in.txt)" == "*_in.txt" ];then
	NUM_OF_FILES=0
fi

sleep 0.1;

echo -e " Input files detected: ${CY}${NUM_OF_FILES}${NC}\n";

if [ $NUM_OF_FILES == 0 ]; then
	echo -e "(${RED}No input files detected${NC})\n${CY}Please make sure script is in folder with input files.${NC}\n";
	sleep 3;
	exit;
fi

num=$NUM_OF_FILES;


# Option menu
echo -e "\nSelect option ( ${CY}ENTER${NC} - Continue | ${CY}r${NC} - delete my_output## files | ${CY}i${NC} - info | ${CY}s${NC} - set new path)
	            ${CY}c${NC} - config";printf "\nOption:";
read remove_ans;

if [ "$remove_ans" == 'c' ];then
	cat .program-tester.conf | grep -E "^(PATH_TO_PRO|SUPPRESS_SA)" > tmp
        cat tmp > .program-tester.conf
	rm tmp
	./program_tester.sh
	exit
fi

if [ "$remove_ans" == "i" ]; then
	echo -e " ______________________________________________________________________________________________\n\n * The program automatically compiles the .c file and compares it with the reference input file\n ${CY}│\n ├── ${NC}Input files needs to end with ${CY}_in.txt\n └── ${NC}For each file ${CY}*_in.txt${NC} there must be ${CY}*_out.txt${NC} file\n\n * Program stores output for each input in ${CY}my_output_##.txt${NC} files\n ${CY}│\n ├── ${NC}Output files can be automatically deleted by program\n${CY} └── ${NC}Differences between outputs are stored in ${CY}diff_outputs.txt${NC}\n\n * Config options\n ${YE}│\n ${YE}├── ${NC}To change config edit file ${CY}.program-tester.conf${NC}\n ${YE}├── ${NC}To disable full program report change value of EXPANDED_REPORT to 0\n${YE} └── ${NC}To disable fsanitizer change value of SUPPRESS_SANITIZER to 1 in ${CY}.program-tester.conf${NC}\n\n ______________________________________________________________________________________________\n";

	echo -e "${CY}Continue?${NC}"
	printf "(y/n):" ; read remove_ans

	if [ "$remove_ans" == 'n' ]; then
		exit 0;
	fi
fi

if [ "$remove_ans" == 's' ]; then
	path_set;
fi

#REMOVE OLD OUTPUT FILE
echo;
for my_output in my_output_*
do
	if [ -f "${my_output}" ]; then
		rm -f ${my_output};
		[ "$remove_ans" == "r" ] && echo -e "removing... ${RED} ${my_output}${NC}" && printf '\033[1A';	
	fi
done


[ "$remove_ans" == "r" ] && echo && echo -e "\n${CY}All removed!${NC}\n";

#Compilation
if [ "$remove_ans" != "r" ]; then
	g++ -Wall -pedantic ${PATH_TO_PROGTEST} -O2 -o pgr_test.exe 2> ERROR_OUT.txt;
	ERROR=$(cat ERROR_OUT.txt);
	if [ ! -z "$ERROR" ]; then
		echo -e "${RED} ** Compilation error! **${NC}";echo;
		g++ -Wall -pedantic ${PATH_TO_PROGTEST} -O2 -o pgr_test.exe;
		exit 0;
	fi
fi
rm -f ERROR_OUT.txt;

SOURCE_PATH=${PATH_TO_PROGTEST};
PATH_TO_PROGTEST="./pgr_test.exe";

# Test inputs on program
if [ "$remove_ans" != "r" ]
then
	#Create debugger exe file
	g++ ${SOURCE_PATH} -g -fsanitize=address,undefined -static-libasan -o pgr_sanitize_a
	#printf '\033[4A';	
	echo -e "      (${CY}Testing inputs${NC})                    \n";

	if [ "$SUPPRESS_SANITIZER" == 1 ];then
		echo -e "${RED}  === fsanitize suppressed ===${NC}\n";
	fi

	rm -fr fails
	stty -echo;
	for input_file in *_in.txt
	do	
		output_file="$(basename -- $input_file "in.txt")out.txt"
		input_lines=$(wc -l < "$input_file")
		[ $input_lines == 0 ] && input_lines=1;
		printf "   ";

		if [ -f $output_file ]; then
			printf "INPUT:\"%s\" \n\n" "$(cat "$input_file")" > my_output_$(printf "%02d\n" "$c").txt;
			
			OUTPUT_TEXT="$( ${PATH_TO_PROGTEST} < "$input_file" | cat - > tmp_output; cat tmp_output)" 

			printf "%s\n" "$OUTPUT_TEXT"  >>  my_output_$(printf "%02d\n" "$c").txt; 			

			#Check if program crashed
			if [ -z "$(cat tmp_output)" ]; then
				
				status_code=$(${PATH_TO_PROGTEST} < "$input_file" ; echo $?)
				if ! [ "$status_code" == '0' ];then
					rm tmp_output pgr_sanitize_a
					echo -e "\n =======================================\n             ${RED}Program crashed!${NC}\n =======================================";
					sleep 0.2;
					
					if [ "$status_code" == '136' ]; then
						status_code="Floating point exception"
					fi
					
					if [ "$status_code" == '139' ]; then
						status_code="Segmentation fault"
					fi
					echo -e "  # my_output_$(printf "%02d\n" "$c").txt"
					echo -e "  ${CY}Input file:${NC} $input_file${CY}\n${CY}  │\n  ├─Exit code:${NC} ${status_code}\n  ${CY}└─Input:${NC} $(cat $input_file) \n _______________________________________\n\n\n";
					
					sleep 1;
					tset;
					exit;
				fi
			fi
			
			# Fsanitize check					
			if [ "$SUPPRESS_SANITIZER" == 0 ];then
			
				./pgr_sanitize_a < "$input_file" 2> tmp_report 1> /dev/null
			
				sanitized="$(cat tmp_report)"
				if ! [ -z "${sanitized}" ];then 
				
					echo -e "\n\n    ${RED}*** fsanitize error ***\n     │\n     └──${YE}Input file:${NC} $input_file\n"
					./pgr_sanitize_a < "$input_file" 1> /dev/null
					sleep 0.5;
					rm pgr_sanitize_a
					FSAN_LINES=$(wc -l < tmp_report)
					echo
				 	tset		
					((FSAN_LINES+=9))
					echo -e "${CY}Suppress fsanitizer?${NC}"; printf "(y/n):";
					read SUPPRESS_SANITIZER
					if [ "$SUPPRESS_SANITIZER" == 'y' ];then 
						SUPPRESS_SANITIZER=1 
					fi
					if [ "$SUPPRESS_SANITIZER" == 'n' ];then 
						SUPPRESS_SANITIZER=0 
					fi
					
					if [ "$SUPPRESS_SANITIZER" == 0 ];then
						rm tmp_report tmp_output 
						exit;
					fi
					
					printf "\033[${FSAN_LINES}A\033[J";echo -e "${RED}  === fsanitize suppressed ===${NC}\n" ;printf "   ";
				fi
			fi

			rm -fr tmp_report;
			rm -fr tmp_output;
			
			# Differences scan
			if [ "$STRICT_ORDER" == 1 ]; then
				
				sed -n "$((2+${input_lines}))~1p" my_output_$(printf "%02d\n" "$c").txt > awk_input_file
				DIFF="$(awk -F "\n" 'NR == FNR {ref_str[FNR] = $0; lines = FNR} NR > FNR {my_lines=FNR;if(FNR > lines){printf("%s[#!separator#!][n!]\n",$0) ;next} if( ref_str[FNR] != $0 ){printf("%s[#!separator#!]%s\n",$0,ref_str[FNR],FNR)}} END {if(my_lines < lines){ for (i = my_lines + 1; i <= lines; i++){printf("%s<-[MIS]\n",ref_str[i])}}}' "$output_file" awk_input_file)"
				rm -fr awk_input_file

			else
				cat ${output_file} | sort > filtered_output
				DIFF="$(diff <(sed -n "$((2+${input_lines}))~1p" my_output_$(printf "%02d\n" "$c").txt | sort ) filtered_output)";	
				rm filtered_output;
			fi
		
			if [ -z "$DIFF" ]; then
				printf "(${GR}Succes${NC})";
				SCORE=$((${SCORE}+1));
			else
				printf "(${RED}Failed${NC})";
				echo "$output_file" >> fails 
			fi
			printf " - my_output_$(printf "%02d\n" "$c")\n";
			sleep 0.01;
		else
			printf "(${YE}Missing output${NC})"; printf " - my_output_$(printf "%02d\n" "$c")\n";sleep 0.01;
			echo -e "INPUT: $(cat $input_file)\n" >> my_output_$(printf "%02d\n" "$c").txt
			${PATH_TO_PROGTEST} < "$input_file" | cat - > tmp_output;cat tmp_output >> my_output_$(printf "%02d\n" "$c").txt; rm tmp_output;
			echo -e "\n\nMissing reference output for $input_file" >> my_output_$(printf "%02d\n" "$c").txt
			SCORE=$((${SCORE}+1));
			rm -f tmp_report;
		fi
		((c+=1));

	done
	rm -f pgr_sanitize_a

	echo; echo -e "        ${YE}Score ${CY}(${NC}${SCORE}${CY}/${NC}${num}${CY})${NC}" ; 
	
	#Execution time measuring
	OLD_NUMBER_FORMAT=$LC_NUMERIC;
	LC_NUMERIC=en_US.UTF-8;
	
	TIMEFORMAT=%R; 
	T_SUM=0
	T_MAX=0
	T_NUM=0
	t=0
	echo;	
	for input_file in *_in.txt
		do
			time (${PATH_TO_PROGTEST}) < "$input_file" 2>time_s 1>/dev/null
			PROGRAM_TIME=$(cat time_s);
			rm -fr time_s;
			T_SUM="$(echo "scale=5; $T_SUM + ${PROGRAM_TIME}" | bc)";
			((T_NUM+=1))
			echo -e "${YE}    Measuring time... (${CY}${T_NUM}${YE}/${CY}${NUM_OF_FILES}${YE})${NC}";
			if (( $(echo "$PROGRAM_TIME > $T_MAX" | bc -l) )); then
				T_MAX=${PROGRAM_TIME};
				T_MAX_IN="${CY}(${NC}my_output_$(printf "%02d\n" "$t").txt${CY})${NC}"
				T_MAX_IN_FILE=$input_file
			fi
			printf '\033[1A';
			((t+=1));
		done

	A_TIME="$(echo "scale=5 ; $T_SUM / $T_NUM" | bc)";
	printf '\033[1A'; 
	printf "\n  ${YE}  Total time:${NC} %.3fs       " "${T_SUM}";
	printf "\n  ${YE}Avarage time:${NC} %.3fs       " "${A_TIME}";
	printf "\n  ${YE}         Max:${NC} %.3fs $T_MAX_IN   \n" "${T_MAX}";
	
	unset TIMEFORMAT; sleep 1;
	
        LC_NUMERIC=$OLD_NUMBER_FORMAT;


	# Differences scan
	k=0;
	for output_file in *_out.txt 
	do
		if ! [ -f $output_file ]; then
			continue;
		fi
		input_file="$(basename -- $output_file "out.txt")in.txt"
		input_lines=$(wc -l < "${input_file}")
		echo "Output n.$(printf "%02d\n" "$k")" >> diff_outputs.txt;	
		diff --color -u <(sed -n "$((2+${input_lines}))~1p" my_output_$(printf "%02d\n" "$k").txt) $output_file >> diff_outputs.txt; echo >> diff_outputs.txt;
	        sleep 0.01;	
	done
		stty echo;
		
	# Expanded report
		if [ "$EXPANDED_REPORT" == '1' ];then
			STOP=0
			g++ $([ "$OPTIMIZED_REPORT" == 1  ] && echo -O2) ${SOURCE_PATH} -pg -o pgr_report
			$(./pgr_report < "$T_MAX_IN_FILE" 1> /dev/null; [ "$?" != "0" ] && echo $var > STOP;)
			sleep 0.3
			if [ -f "STOP" ];then
				echo -e "\n(${RED}Warning${NC}) Program ended with error (${CY}${T_MAX_IN_FILE}${NC}) no report available"
				STOP=1
				rm STOP
			fi
			if [ "$STOP" == '0' ];then
				echo -e "\n${CY} Full program report$([ "$OPTIMIZED_REPORT" == 1  ] && echo -e "${NC} (${GR}OPTIMIZED${NC})${CY}" || echo -e "${NC} (${PUR}NO OPTIMIZATION${NC})${CY}"):${NC}"
				# Memory info
				MEM_USAGE=$(command time -v ./pgr_test.exe < "$T_MAX_IN_FILE" 2>&1 1>/dev/null | head -10 | tail -1 | rev | cut -d" " -f1 |rev)
				if [ ! -z "$(echo "$MEM_USAGE" | grep "found")" ];then
					echo -e "\n  ${CY}(${RED}ERROR${CY})${NC} Command time not installed, please install \"time\" package to see memory usage\n";
				else
					echo -e "${PUR} │\n └─>${YE} Memory allocated for program${NC}: ${MEM_USAGE} KB ${CY}(${NC}$(((${MEM_USAGE}*1000/1024))) KiB${CY})${NC}"
				fi

				gprof --brief --flat-profile pgr_report gmon.out | tail +2 | sed -n 's/^/    /p'
			fi
			rm -fr pgr_report gmon.out

		fi



	# Show differences
	if [ ! $SCORE == "$num" ]; then
		echo;  
		echo -e "${CY}Show differences?${NC}"; printf "(y/n):"
		read ANS2;
		if [ "$ANS2" = "y" ]; then
			echo -e "\n———————————————————————————————\n ${RED}MY_OUTPUT ${NC}|| ${GR}REFERENCE Output${NC}\n———————————————————————————————";			
			stty -echo;
			if [ $STRICT_ORDER == 1 ];then
				echo -e " ${CY}Mode:${NC} Strict order\n"
			fi
			
			if [ $STRICT_ORDER == 0 ];then
				echo -e " ${CY}Mode:${NC} Ignore order\n"
			fi
			echo -e "${NC}================================";
			
			for output_file in *_out.txt
			do
				((h+=1));
				
				if [ ! -f "$output_file" ]; then
					continue
				fi
			
				if [ -z "$(grep -E "^$output_file" fails)" ];then
					continue
				fi
			
				input_file="$(basename -- $output_file "out.txt")in.txt";
				input_lines="$(wc -l < "$input_file")";
				[ $input_lines == 0 ] && input_lines=1;	
				
				# Differences print
				if [ $STRICT_ORDER == 1 ] && [ $DEBUG == 0 ];then
					
					echo -e "${CY}#${NC} My output n.$(printf "%02d\n" "$h")";
					sed -n "$((2+${input_lines}))~1p" my_output_$(printf "%02d\n" "$h").txt > awk_input_file
					echo -e "${CY}Reference:${NC} $output_file"; echo
					#diff --color <(sed -n "$((2+${input_lines}))~1p" my_output_$(printf "%02d\n" "$h").txt) ${output_file}
					awk -F "\n" 'NR == FNR {ref_str[FNR] = $0; lines = FNR} NR > FNR {my_lines=FNR;if(FNR > lines){printf("%s[#!separator#!][n!]\n",$0) ;next} if( ref_str[FNR] != $0 ){printf("%s[#!separator#!]%s\n",$0,ref_str[FNR],FNR)}} END {if(my_lines < lines){ for (i = my_lines + 1; i <= lines; i++){printf("%s<-[MIS]\n",ref_str[i])}}}' "$output_file"  awk_input_file | sed -E 's/(\[#!separator#!\])$/\1[newline]/' | sed -E 's/\[#!separator#!\]\[n!\]/[n!]/' | sed -E "s/^(.*)<-\[MIS\]$/$(echo -e ${PUR})\1 $(echo -e ${NC})[$(echo -e ${YE})missing$(echo -e ${NC})]/ ; s/^/$(echo -e ${RED})/; s/\[#!separator#!\]/$(echo -e ${NC})<—$(echo -e ${GR})/;" | sed -E " s/\[n\!\]/$(echo -e ${YE})∅/; s/$/$(echo -e ${NC})/" | sed -E "s/\[newline\]/$(echo -e ${YE})newline$(echo -e ${NC})/"
					rm -fr awk_input_file
					
					echo -e "\n${NC}================================";	
					echo;
				fi
				
				if [ $DEBUG == 0 ] && [ $STRICT_ORDER == 0 ];then

					cat "$output_file" | sort | uniq -c | sed -E 's/^[ ]+//; s/^([0-9]+) /\1#===#/' > awk_ref
                                        sed -n "$((2+${input_lines}))~1p" my_output_$(printf "%02d\n" "$h").txt > tmp_file
                                        cat tmp_file | sort | uniq -c | sed -E 's/^[ ]+//; s/^([0-9]+) /\1#===#/' > awk_my

                                        # AWK -- START
                                        awk -F "#===#" 'NR == FNR {str[NR]= $2; num[NR] = $1} NR > FNR { for(i in str){if(bad[FNR] != "#@DONE"){ bad[FNR] = $2} ;if(str[i] == $2 && num[i] == $1){printf("[0]:%s\n",$2);bad[FNR] = "#@DONE"; str[i] = "#@DONE" } if(str[i] == $2 && num[i] != $1){printf("[%d]:%s\n",$1-num[i],$2); bad[FNR] = "#@DONE"; str[i] = "#@DONE"}}} END {for (i in bad){if(bad[i] != "#@DONE"){printf("[BAD]:%s\n",bad[i])}}    for(j in str){ if( str[j] != "#@DONE" ){printf("[MIS]:%s\n",str[j])}}}' awk_ref awk_my > awk_result
                                        # AWK -- END
                                        rm -fr tmp_file awk_ref awk_my
					
					# Bad lines parse
					cat awk_result | grep -E "^\[[1-9][0-9]*\]:" | sed -E 's/^\[(.+)\]:.*/\1/;' > bad_lines_nums
					cat awk_result | grep -E "^\[[1-9][0-9]*\]:" | sed -E 's/^\[(.+)\]://;' > bad_lines_str
					paste -d "~" bad_lines_str bad_lines_nums | sed -E "s/~([1-9][0-9]*)$/$(echo -e "${CY}")\[$(echo -e "${YE}")\1x more$(echo -e "${CY}")\]$(echo -e "${NC}")/" > bad_lines
					rm -fr bad_lines_nums bad_lines_str
					
					# Missing lines parse (not enough lines)
					cat awk_result | grep -E "^\[-[0-9]+\]:" | sed -E 's/^\[(.+)\]:.*/\1/;' > bad_lines_nums
					cat awk_result | grep -E "^\[-[0-9]+\]:" | sed -E 's/^\[(.+)\]://;' > bad_lines_str
					paste -d "~" bad_lines_str bad_lines_nums | sed -E "s/~-([0-9]+)$/$(echo -e "${CY}")\[$(echo -e "${YE}")\1x$(echo -e "${CY}")\]$(echo -e "${NC}")/" > missing_lines
					rm -fr bad_lines_nums bad_lines_str
					
					
					cat awk_result | grep -E "^\[BAD\]:" | sed -E 's/^\[BAD\]://; s/$//' >> bad_lines
					
					# Missing lines parse (completely missing)
					cat awk_result | grep -E "^\[MIS\]:" | sed -E 's/^\[MIS\]://;' >> missing_lines
					

					echo -e "${CY}#${NC} My output n.$(printf "%02d\n" "$h")";
					echo -e "${CY}Reference:${NC} ${output_file}"
					if [ ! -z "$(cat bad_lines)" ];then
						echo -e "\n${CY}(${RED}Bad lines${CY})${NC}\n$(sed -E "s/^$|^[ \t]+$/($(echo -e ${YE})whitespace$(echo -e ${NC}))/;s/^/: /" bad_lines )\n"
					fi
					if [ ! -z "$(cat missing_lines)" ];then
						echo -e "\n${CY}(${GR}Missing lines${CY})${NC}\n$(  sed -E "s/^$|^[ \t]+$/($(echo -e ${YE})whitespace$(echo -e ${NC}))/;s/^/: /" missing_lines )\n"
					fi
					echo -e "\n${NC}================================";	
			
					rm -fr awk_result bad_lines missing_lines bad_lines_nums bad_lines_str
				fi

				if [ $DEBUG == 1 ];then
					cat "$output_file" | sort | uniq -c | sed -E 's/^[ ]+//; s/^([0-9]+) /\1#===#/' > awk_ref
					sed -n "$((2+${input_lines}))~1p" my_output_$(printf "%02d\n" "$h").txt > tmp_file
					cat tmp_file | sort | uniq -c | sed -E 's/^[ ]+//; s/^([0-9]+) /\1#===#/' > awk_my
					echo -e "# My output n.$(printf "%02d\n" "$h")";
					echo -e "\n  (${YE}Debug mode active${NC})\n"

					# AWK -- START
					awk -F "#===#" 'NR == FNR {str[NR]= $2; num[NR] = $1} NR > FNR { for(i in str){if(bad[FNR] != "#@DONE"){ bad[FNR] = $2} ;if(str[i] == $2 && num[i] == $1){printf("[0]:%s\n",$2);bad[FNR] = "#@DONE"; str[i] = "#@DONE" } if(str[i] == $2 && num[i] != $1){printf("[%d]:%s\n",$1-num[i],$2); bad[FNR] = "#@DONE"; str[i] = "#@DONE"}}} END {for (i in bad){if(bad[i] != "#@DONE"){printf("[BAD]:%s\n",bad[i])}}    for(j in str){ if( str[j] != "#@DONE" ){printf("[MIS]:%s\n",str[j])}}}' awk_ref awk_my | sed -E "s/^\[0\]/[$(echo -e ${GR})0$(echo -e ${NC})]/" | sed -E "s/^\[BAD\]/[$(echo -e ${RED})BAD$(echo -e ${NC})]/" | sed -E "s/^\[MIS\]/[$(echo -e ${YE})MIS$(echo -e ${NC})]/"

					# AWK -- END

					rm -fr tmp_file awk_ref awk_my
					echo -e "\n____________________";
				fi


			done
			rm -fr fails
			stty echo;
		fi
	fi
	rm -fr fails
	# Valgrind test
	if [ $DISABLE_VALGRIND != 1 ];then
		echo; echo -e "${CY}Test program on valgrind?${NC}";printf "(y/n):";
		read CHOICE;
		if [ "$CHOICE" == 'y' ]; then
				g++ -Wall -pedantic -O0 -g ${SOURCE_PATH} -o pgr_memory_test;
			echo -e "——————————————————————————————————————————————————————————————————————————————\n\n";
				valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./pgr_memory_test < "$T_MAX_IN_FILE" 1>/dev/null;
				echo;
				sleep 0.2;
				rm pgr_memory_test;
		fi
	fi
fi

