#!/bin/bash

echo "converting spaces to tabs.."
find ./src ./include -type f -name "*.c" -o -name "*.h" | xargs sed -i 's/    /\t/g'

echo "trim spaces.."
find ./src ./include -type f -name "*.c" -o -name "*.h" | xargs sed -i 's/[ \t]*$//'

echo "formating comments.."
find ./src ./include -type f \( -name "*.h" -o -name "*.c" \) -exec sed -i -E 's|//(.*)|/*\1*/|' {} +