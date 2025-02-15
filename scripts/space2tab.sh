echo "converting spaces to tabs.."

find ../src ../include -type f -name "*.c" -o -name "*.h" | xargs sed -i 's/    /\t/g'