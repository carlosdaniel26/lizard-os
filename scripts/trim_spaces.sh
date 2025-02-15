echo "trim spaces"

find ../src ../include -type f -name "*.c" -o -name "*.h" | xargs sed -i 's/[ \t]*$//'