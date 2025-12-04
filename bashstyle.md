Bash style to follow here:

  * Quote everything
  
  * Use curly braces, i.e. ${...}, for variables, except $# $? $$ $! $1 $2 ...
  * Use 4-space indentation (no tab)
  
  * Put "; then" and "; do" on same line as "if" and "while" or "for", resp.
    For "fi" and "done" (as well as "esac") on their own line
    
  * Single equality sign in tests
  
  * Use full parameter names where possible (i.e. "--force", not "-f")
  
  * Use double brackets for tests. Only one set of brackets per if statement
    i.e. "[[ X && B ]]"  not "[[ X ]] && [[ B ]]".
    
  * Run the script through shellcheck and heed all warnings and errors.
    It should run without warnings and errors, although it acceptable to
    have "#shellcheck disable=SC1090" in the file.
