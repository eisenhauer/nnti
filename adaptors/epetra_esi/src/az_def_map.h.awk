($1=="#define" && $3 ~ /[0-9]/) && ($3 !~ /[()]/) { print "\"" $2 " " $3 "\","}
