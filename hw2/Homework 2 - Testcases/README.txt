Test Cases:

0.flow
Action: path
Command: pwd
Description: Just one node in any .flow file.

1.flow
Action: doit
Command: ls | wc
Description: Normal pipe. 

2.flow
Action: doit
Command: ls -l | wc
Description: Checks if - arguments can be handled properly. 

3.flow
Action: doit
Command: ls -l | ls
Description: Checks if a pipe can be used for a "to" node that doesn't usually take inputs. 

4.flow
Action: doit
Command: echo foo | cat
Description: No quotes for argument. 

5.flow
Action: doit
Command: echo 'foo' | cat
Description: Single quotes for arguments.

6.flow
Action: doit
Command: echo "foo" | cat
Description: Double quotes for arguments. 

7.flow
Action: doit
Command: echo 'f o o' | cat
Description: Quotes with space. 

8.flow
Action: doit
Command: ls ; pwd
Description: Normal concat. 

9.flow
Action: doit
Command: ls; ls ; ls -a
Description: Concat with more than 2 parts and also arguments. 

10.flow
Action: doit
Command: echo foo1 ; echo 'foo2' ; echo "foo3" ; echo 'f o o 4'
Description: No quotes, single quotes, double quotes, and spaces. 

11.flow
Action: doit
Command: ls | wc ; pwd
Description: Mix. 

12.flow
Action: doit
Command: ( cat foo.txt ; cat foo.txt | sed s/o/u/g ) | wc
Description: Complicated mix.

13.flow
Action: doit
Command: ( seq 1 5 | awk '{print $1*$1}'; seq 1 5 | awk '{print $1*2}'; seq 1 5 | awk '{print $1+5}' ) | sort -n | uniq
Description: Additional Extra credit but counted only if previous 13 test cases were all passed.

14.flow
Action: doit
Command: mkdir a 2>&1
Description: calling stderr directly (like a node)

15.flow
Action: doit
Command: mkdir a 2>&1 | wc
Description: Calling non-stderr as action. 

16.flow
Action: doit
Command: ls > output.txt
Description: write

17.flow
Action: doit
Command: cat foo.txt | wc
Description: read

- If 2 or more test cases submitted by the student in the test_cases.txt is valid (working and not the same as prrovided in the Assignment by the Prof), the student gets 2 marks.
- Test cases from 0 to 12 i.e. 13 test cases are for Full Credit.
- Test case 13 is an additional extra credit for students who passed each and every test cases before for Full Credit i.e. 0.flow to 12.flow.
- Test cases 14 and 15 are for error handling & Test Cases 16 and 17 are for file redirection.

Required marks for Full Credit: 2 + 13 = 15 (100%)
Maximum Possible Marks: 2+ 13 + 1 + 4 = 20 (133.33%)